const { app, BrowserWindow } = require("electron");
const path = require("node:path");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

// --- 1. SETUP WEB SERVER (Express + Socket.io) ---
const express = require('express');
const http = require('http');
const { Server } = require("socket.io");

const expressApp = express();
const server = http.createServer(expressApp);
const io = new Server(server, {
  cors: { origin: "*", methods: ["GET", "POST"] }
});

// ให้ Server มองเห็นไฟล์ในโฟลเดอร์ปัจจุบัน (เพื่อให้โหลด index.html, logo.png ได้)
expressApp.use(express.static(__dirname));

// เมื่อเข้าเว็บ (http://localhost:3000) ให้ส่งไฟล์ index.html ไปแสดง
expressApp.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

// เริ่ม Server ที่ Port 3000
const PORT = 3000;
server.listen(PORT, '0.0.0.0', () => { // '0.0.0.0' เพื่อให้เครื่องอื่นในวง LAN เข้าได้
  console.log(`🚀 Web Server running at http://localhost:${PORT}`);
});

// --- 2. SERIAL PORT LOGIC ---
let gsSerialPort = null;
let parser = null;

// ฟังก์ชันเชื่อมต่อ Serial Port
function openSerialPort(portPath, baudRate) {
    if (gsSerialPort && gsSerialPort.isOpen) {
        gsSerialPort.close(); // ปิดอันเก่าก่อนถ้ามี
    }

    gsSerialPort = new SerialPort({ path: portPath, baudRate: parseInt(baudRate) || 115200 }, (err) => {
        if (err) {
            console.error("Error opening port:", err.message);
            io.emit('err', 'Open Error: ' + err.message);
            return;
        }
    });

    // เมื่อพอร์ตเปิดสำเร็จ
    gsSerialPort.on('open', () => {
        console.log(`✅ Serial Port Opened: ${portPath}`);
        io.emit('status', 'CONNECTED'); // บอกหน้าเว็บว่าต่อติดแล้ว
    });

    // ตั้งค่าตัวอ่านข้อมูล (Parser)
    parser = gsSerialPort.pipe(new ReadlineParser({ delimiter: '\n' })); // ตัดคำเมื่อขึ้นบรรทัดใหม่
    
    // เมื่อมีข้อมูลเข้ามาจากบอร์ด
    parser.on('data', (data) => {
        // console.log('Data:', data); // (ปิดไว้จะได้ไม่รก Terminal)
        io.emit('serial-data', data); // ส่งข้อมูลไปหน้าเว็บทันที
    });

    // จัดการ Error
    gsSerialPort.on('error', (err) => {
        console.error('Serial Error:', err.message);
        io.emit('err', err.message);
    });
}

// --- 3. SOCKET.IO EVENTS (เชื่อมหน้าเว็บกับโปรแกรม) ---
io.on('connection', (socket) => {
    // 3.1 เว็บขอรายชื่อ Port
    socket.on('list-ports', async () => {
        try {
            const ports = await SerialPort.list();
            socket.emit('ports-list', ports);
        } catch (err) {
            console.error(err);
        }
    });

    // 3.2 เว็บสั่งเชื่อมต่อ
    socket.on('connect-serial', ({ path, baudRate }) => {
        console.log(`Request connection to ${path} at ${baudRate}`);
        openSerialPort(path, baudRate);
    });

    // 3.3 เว็บสั่งส่งข้อมูล (Uplink)
    socket.on('write-serial', (data) => {
        if(gsSerialPort && gsSerialPort.isOpen) {
            console.log("Writing to Serial:", data);
            gsSerialPort.write(data, (err) => {
                if(err) console.error("Write Error:", err);
            });
        }
    });
});

// --- 4. ELECTRON WINDOW ---
let mainWindow;

const createWindow = () => {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true
    },
    // ซ่อน Title Bar เดิมของ Windows/Mac เพื่อความสวยงาม
    titleBarStyle: "hidden", 
    titleBarOverlay: {
      color: "#050a10",
      symbolColor: "#f36c21",
      height: 60
    },
    icon: path.join(__dirname, 'logo.png')
  });

  // โหลดหน้าเว็บผ่าน Localhost (เพื่อให้ Socket.io ทำงานสมบูรณ์)
  mainWindow.loadURL(`http://localhost:${PORT}`);
};

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});