const { app, BrowserWindow } = require('electron');
const { spawn } = require('child_process');
const path = require('path');

let engineProcess;

function createWindow() {
  const win = new BrowserWindow({ 
    width: 1280, 
    height: 720,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });
  
  // In development, we might use a dev server
  // win.loadURL('http://localhost:5173');
  win.loadFile('index.html');
  
  // Spawn Engine Sidecar
  // Note: Path depends on build configuration and where Electron is run from
  const enginePath = path.join(__dirname, '../../Bin/RelWithDebInfo/InnocenceEngine.exe');
  engineProcess = spawn(enginePath, ['-sidecar', '-renderer 0', '-loglevel 0']);

  engineProcess.stdout.on('data', (data) => {
    console.log(`Engine: ${data}`);
  });

  engineProcess.stderr.on('data', (data) => {
    console.error(`Engine Error: ${data}`);
  });
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (engineProcess) {
    engineProcess.kill();
  }
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
