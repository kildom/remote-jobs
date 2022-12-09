
import * as net from 'net';


function serverError(error) {
	console.error('Server error: ', error);
}

function serverListening() {
	console.log('Server listening');
}

function serverConnection(socket) {
	console.log('Server connected: ', socket);
}

function serverClosed() {
	console.log('Server closed');
}

async function main() {
	let socket = net.createConnection('/tmp/test_str343')
		.on('connect', () => console.log('Connected'))
		.on('error', error => console.error('Error: ', error))
		.on('close', hadError => console.error('Close: with error=', hadError));
	setTimeout(() => socket.destroy(), 7000);
}

main();
