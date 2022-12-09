

import * as net from 'net';
import * as fs from 'fs';
import * as path from 'path';
import { TextDecoder, TextEncoder } from 'util';


const CONNECTION_PREFIX =
	process.platform == 'win32' ? '\\\\?\\pipe\\RemJobs75oKmnN7rWX'
	: '/tmp/RemJobs75oKmnN7rWX';

function serverError(error: any) {
	console.error('Server error: ', error);
}

function serverListening() {
	console.log('Server listening');
}

class SocketCloseError extends Error { }
class SocketTimeoutError extends Error { }

const STUB_RECV_TIMEOUT = 10000;

class SocketPromiseWrapper {

	private signalListeners: [any, any][] = [];
	private timeout: any = null;
	private buffers: Buffer[] = [];
	private firstBufferUsed: number = 0;
	private error: Error | null = null;
	private socket: net.Socket | null = null;
	private closeResolve: any = null;
	private closeReject: any = null;
	private viewArray: Uint8Array = new Uint8Array(16);
	private view: DataView;
	private dec: TextDecoder = new TextDecoder();
	private enc: TextEncoder = new TextEncoder();

	public constructor(socket: net.Socket) {
		this.view = new DataView(this.viewArray.buffer, this.viewArray.byteOffset);
		this.socket = socket
			.on('close', (hadError) => {
				if (hadError) {
					this.closeReject(new SocketCloseError())
				} else {
					this.closeResolve();
				}
			})
			.on('data', (data) => {
				//console.log('Data', data.length);
				this.buffers.push(data.subarray());
				this.signal();
			})
			.on('end', () => console.log('end'))
			.on('error', () => (err: Error) => {
				this.error = err;
				this.signal();
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
						(this.signalListeners.pop() as any)[1](new SocketTimeoutError());
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

	public close() {
		if (this.socket === null) return;
		return new Promise<void>((resolve, reject) => {
			this.closeResolve = resolve;
			this.closeReject = reject;
			this.socket!.destroy();
			this.socket = null;
		});
	}

	public send(data: string | Uint8Array | Buffer) {
		if (this.socket === null) throw Error('Not connected');
		return new Promise<void>((resolve, reject) => {
			this.socket!.write(data, err => {
				if (err) {
					reject(err);
				} else {
					resolve();
				}
			});
		});
	}

	public async sendUint32(value: number) {
		this.view.setUint32(0, value, true);
		await this.send(this.viewArray.subarray(0, 4));
	}

	public async sendString(value: string) {
		let buf = this.enc.encode(value);
		await this.sendUint32(buf.byteLength);
		await this.send(buf);
	}

	private async recvPartial(output: Uint8Array, length: number, offset: number) {
		//console.log('recvPartial', length, offset);
		while (this.buffers.length == 0 && this.error === null) {
			await this.waitForSignal();
		}
		if (this.error) {
			let error = this.error;
			this.error = null;
			throw error;
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
		//console.log('recvPartial', copyBytes);
		return copyBytes;
	}

	public async recv(output: Uint8Array, length: number, offset: number = 0) {
		//console.log('recv', length, offset);
		while (length > 0) {
			let done = await this.recvPartial(output, length, offset);
			length -= done;
			offset += done;
		}
		//console.log('recv exit');
	}

	public async recvUint32() {
		await this.recv(this.viewArray, 4);
		return this.view.getUint32(0, true);
	}

	public async recvString() {
		let length = await this.recvUint32();
		let buf = new Uint8Array(length);
		await this.recv(buf, length);
		return this.dec.decode(buf);
	}
}

async function handleClient(w: SocketPromiseWrapper) {
	try {
		let magic = await w.recvUint32();
		if (magic != 0x7F4A9400) throw Error('Unsupported stub version.');
		let argc = await w.recvUint32();
		let argv = new Array(argc);
		for (let i = 0; i < argc; i++) {
			argv[i] = await w.recvString();
			console.log('arg', i, argv[i]);
		}
		let cwd = await w.recvString();
		console.log('cwd', cwd);
		let length = await w.recvUint32();
		let hash = new Uint8Array(length);
		await w.recv(hash, length);
		console.log('hash', Buffer.from(hash).toString('hex'));
		await w.sendUint32(1);
		await w.sendString("This is the stdout.\n");
		await w.sendUint32(2);
		await w.sendString("This is the stderr.\n");
		//await new Promise(r => setTimeout(r, 1000));
		await w.sendUint32(3);
		let envc = await w.recvUint32();
		let envv = new Array(envc);
		console.log('envc', envc);
		for (let i = 0; i < envc; i++) {
			envv[i] = await w.recvString();
			console.log('envv', i, envv[i]);
		}
		await w.sendUint32(0);
		await w.sendUint32(33);
	} finally {
		await w.close();
	}
}

function serverConnection(socket: net.Socket) {
	console.log('Server connected');
	let w = new SocketPromiseWrapper(socket);
	handleClient(w);
}

function serverClosed() {
	console.log('Server closed');
}

async function main() {
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
