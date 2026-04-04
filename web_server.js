const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// ให้ Server อ่านไฟล์ในโฟลเดอร์ปัจจุบัน
app.use(express.static(__dirname));

// Route หลักส่งไฟล์ index.html
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

let port = null;

io.on('connection', (socket) => {
    console.log('User connected via Web');

    // 1. ส่งรายชื่อ Port
    socket.on('list-ports', async () => {
        try {
            const ports = await SerialPort.list();
            socket.emit('ports-list', ports);
        } catch (err) { console.error(err); }
    });

    // 2. เชื่อมต่อ Serial
    socket.on('connect-serial', ({ path, baudRate }) => {
        if (port && port.isOpen) port.close();
        try {
            console.log(`Connecting to ${path} at ${baudRate}...`);
            port = new SerialPort({ path: path, baudRate: parseInt(baudRate) });
            const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

            port.on('open', () => {
                console.log('Serial Port Opened');
                io.emit('status', 'CONNECTED');
            });

            parser.on('data', (data) => {
                io.emit('serial-data', data);
            });

            port.on('error', (err) => {
                console.error('Serial Error:', err.message);
                socket.emit('err', err.message);
            });
        } catch (err) {
            socket.emit('err', err.message);
        }
    });

    // 3. ส่งคำสั่ง (Uplink)
    socket.on('write-serial', (data) => {
        if (port && port.isOpen) {
            port.write(data, (err) => {
                if(err) console.error('Write Error:', err);
                else console.log('Uplink:', data.trim());
            });
        }
    });
});

server.listen(3000, () => {
    console.log('🚀 TCE SAT SERVER READY -> http://localhost:3000');
});