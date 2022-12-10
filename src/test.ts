

import * as net from 'net';
import * as fs from 'fs';
import * as path from 'path';
import { TextDecoder, TextEncoder } from 'util';
import { Mutex } from 'async-mutex';

const mutexMember = Symbol();

function synchronized(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
	let func = descriptor.value;
	descriptor.value = async function (...args: any[]) {
		let mutex: Mutex = this[mutexMember];
		if (!mutex) {
			mutex = new Mutex();
			this[mutexMember] = mutex;
		}
		let release = await mutex.acquire();
		try {
			return await func.apply(this, args);
		} finally {
			release();
		}
	}
}

const CONNECTION_PREFIX =
	process.platform == 'win32' ? '\\\\?\\pipe\\RemJobs75oKmnN7rWX'
		: '/tmp/RemJobs75oKmnN7rWX';

function serverError(error: any) {
	console.error('Server error: ', error);
}

function serverListening() {
	console.log('Server listening');
}
const STUB_RECV_TIMEOUT = 10000;

const MAX_ENVIRONMENT_CACHE_SIZE = 5 * 1024 * 1024;

let environmentCache: { [hash: string]: [number, string[]] } = {};
let environmentCacheSize = 0;

function getCachedEnvironment(hash: string): string[] | null {
	if (hash in environmentCache) {
		let env = environmentCache[hash];
		delete environmentCache[hash];
		environmentCache[hash] = env;
		return env[1];
	} else {
		return null;
	}
}

function addCachedEnvironment(hash: string, value: string[]) {
	let size = 2 * value.reduce((sum, x) => sum + x.length, 0) + 64 * value.length;
	while (environmentCacheSize > 0 && environmentCacheSize + size > MAX_ENVIRONMENT_CACHE_SIZE) {
		let oldestHash: string | null = null;
		for (let h in environmentCache) {
			oldestHash = h;
			break;
		}
		if (oldestHash !== null) {
			environmentCacheSize -= environmentCache[oldestHash][0];
			delete environmentCache[oldestHash];
		}
	}
	environmentCache[hash] = [size, value];
	environmentCacheSize += size;
}


class StubTool {

	/*
	States:
		ACTIVE: socket != null && error == null
		ERROR: socket == null && error != null
		CLOSED: socket == null && error == null
		[invalid]: socket != null && error != null
	*/

	private signalListeners: [any, any][] = [];
	private timeout: any = null;
	private buffers: Buffer[] = [];
	private error: Error | null = null;
	private firstBufferUsed: number = 0;
	private socket: net.Socket | null = null;
	private viewArray: Uint8Array = new Uint8Array(16);
	private view: DataView;
	private dec: TextDecoder = new TextDecoder();
	private enc: TextEncoder = new TextEncoder();
	private toolArgs: string[] = [];
	private toolCwd: string = '';
	private toolEnv: string[] = [];

	public constructor(socket: net.Socket) {
		this.view = new DataView(this.viewArray.buffer, this.viewArray.byteOffset);
		this.socket = socket
			.on('close', hadError => {
				if (this.socket !== null) {
					let s = this.socket;
					this.socket = null;
					s.destroy();
				}
				if (hadError && this.error === null) {
					this.error = new Error('Socket close error.');
				}
				this.signal();
			})
			.on('data', data => {
				this.buffers.push(data.subarray());
				this.signal();
			})
			.on('error', err => {
				this.setError(err);
			});
		this.socket.resume();
	}

	private waitForSignal() {
		return new Promise((resolve, reject) => {
			this.signalListeners.push([resolve, reject]);
			if (this.timeout === null) {
				this.timeout = setTimeout(() => {
					this.timeout = null;
					while (this.signalListeners.length > 0) {
						(this.signalListeners.pop() as any)[1](new Error('Socket timeout error.'));
					}
				}, STUB_RECV_TIMEOUT);
			}
		})
	}

	private signal() {
		if (this.timeout !== null) {
			clearTimeout(this.timeout);
			this.timeout = null;
		}
		while (this.signalListeners.length > 0) {
			(this.signalListeners.pop() as any)[0]();
		}
	}

	private setError(err: Error) {
		if (this.error === null) {
			this.error = err;
			if (this.socket !== null) {
				let s = this.socket;
				this.socket = null;
				s.destroy();
			}
			this.signal();
		}
		return err;
	}

	private async close() {
		if (this.socket !== null) {
			this.socket.destroy();
			while (this.socket !== null) {
				await this.waitForSignal();
			}
		}
	}

	private send(data: string | Uint8Array | Buffer) {
		if (this.socket === null) {
			throw Error('Sending data to disconnected socket.');
		}
		return new Promise<void>((resolve, reject) => {
			this.socket!.write(data, err => {
				if (err) {
					reject(this.setError(err));
				} else {
					resolve();
				}
			});
		});
	}

	private async sendUint32(value: number) {
		this.view.setUint32(0, value, true);
		await this.send(this.viewArray.subarray(0, 4));
	}

	private async recvPartial(output: Uint8Array, length: number, offset: number) {
		while (this.buffers.length == 0) {
			if (this.error !== null) {
				throw this.error;
			} else if (this.socket === null) {
				throw new Error('Socket disconnected.');
			}
			await this.waitForSignal();
		}
		let buffer = this.buffers[0];
		let srcOffset = this.firstBufferUsed;
		let copyBytes = buffer.length - srcOffset;
		if (copyBytes > length) {
			copyBytes = length;
			this.firstBufferUsed += copyBytes;
		} else {
			this.firstBufferUsed = 0;
			this.buffers.shift();
		}
		output.set(new Uint8Array(buffer.buffer, buffer.byteOffset + srcOffset, copyBytes), offset);
		return copyBytes;
	}

	private async recv(output: Uint8Array, length: number, offset: number = 0) {
		while (length > 0) {
			let done = await this.recvPartial(output, length, offset);
			length -= done;
			offset += done;
		}
	}

	private async recvUint32() {
		await this.recv(this.viewArray, 4);
		return this.view.getUint32(0, true);
	}

	private async recvString() {
		let length = await this.recvUint32();
		let buf = new Uint8Array(length);
		await this.recv(buf, length);
		return this.dec.decode(buf);
	}

	@synchronized
	public async init() {
		try {
			let magic = await this.recvUint32();
			if (magic != 0x7F4A9400) throw Error('Unsupported stub-tool version.');
			let argc = await this.recvUint32();
			this.toolArgs = new Array(argc);
			for (let i = 0; i < argc; i++) {
				this.toolArgs[i] = await this.recvString();
			}
			this.toolCwd = await this.recvString();
			let length = await this.recvUint32();
			let hashBinary = new Uint8Array(length);
			await this.recv(hashBinary, length);
			let hash = Buffer.from(hashBinary).toString('hex');
			let cachedEnv = getCachedEnvironment(hash);
			if (cachedEnv === null) {
				await this.sendUint32(3);
				let envCount = await this.recvUint32();
				this.toolEnv = new Array(envCount);
				for (let i = 0; i < envCount; i++) {
					this.toolEnv[i] = await this.recvString();
				}
				addCachedEnvironment(hash, this.toolEnv);
			} else {
				this.toolEnv = cachedEnv;
			}
		} catch (err) {
			throw this.setError(err);
		}
	}

	@synchronized
	public async print(value: Uint8Array, stderr: boolean) {
		try {
			await this.sendUint32(stderr ? 2 : 1);
			await this.sendUint32(value.length);
			await this.send(value);
		} catch (err) {
			throw this.setError(err);
		}
	}

	@synchronized
	public async exit(status: number) {
		try {
			await this.sendUint32(0);
			await this.sendUint32(status);
			await this.close();
		} catch (err) {
			throw this.setError(err);
		}
	}

	public get env() {
		return this.toolEnv;
	}

	public get cwd() {
		return this.toolCwd;
	}

	public get args() {
		return this.toolArgs;
	}
}

async function handleClient(socket: net.Socket) {
	let tool = new StubTool(socket);
	await tool.init();
	for (let arg of tool.args) {
		console.log('arg', arg);
	}
	console.log('cwd', tool.cwd);
	for (let env of tool.env) {
		console.log('env', env);
	}
	let enc = new TextEncoder();
	await tool.print(enc.encode("This is stdout.\n"), false);
	await tool.print(enc.encode("This is stderr.\n"), true);
	await tool.exit(13);
}

function serverConnection(socket: net.Socket) {
	console.log('Server connected');
	handleClient(socket);
}

function serverClosed() {
	console.log('Server closed');
}

async function main() {
	try {
		fs.mkdirSync(CONNECTION_PREFIX, { recursive: true });
	} catch { }
	let server = net.createServer()
		.on('error', serverError)
		.on('listening', serverListening)
		.on('connection', serverConnection)
		.on('close', serverClosed);
	try {
		fs.unlinkSync(`${CONNECTION_PREFIX}${path.sep}0S`);
	} catch { }
	server.listen(`${CONNECTION_PREFIX}${path.sep}0S`);
	setTimeout(() => server.close(), 20000);
}

main();
