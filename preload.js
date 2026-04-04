const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
    // รับข้อมูลจาก Main Process
    onUpdateData: (callback) => ipcRenderer.on('update-data', (event, value) => callback(value)),
    
    // ขอรายชื่อ Serial Port
    listPorts: () => ipcRenderer.invoke('list-ports'),
    
    // สั่งเชื่อมต่อ Serial Port (แก้ใหม่ ส่งทั้ง Path และ BaudRate)
    connectSerial: (path, baudRate) => ipcRenderer.send('connect-serial', { path, baudRate }),
    
    // ส่งข้อมูลอื่นๆ
    send: (channel, data) => ipcRenderer.send(channel, data),
    
    getVersions: () => process.versions
});