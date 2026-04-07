const { app, BrowserWindow, ipcMain, Menu, nativeImage } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const WebSocket = require('ws');

let engineProcess;
let win;
let socket;

function spawnEngine() {
  if (engineProcess) {
    console.log('Main: Engine already running, skipping spawn.');
    return;
  }

  // Determine engine path from args or default to Main
  let engineExeName = 'Main.exe';
  const engineArg = process.argv.find(arg => arg.startsWith('--engine='));
  if (engineArg) {
    engineExeName = engineArg.split('=')[1] + '.exe';
  }

  const binDir = path.join(__dirname, '../../Bin');
  const enginePath = path.join(binDir, 'RelWithDebInfo/', engineExeName);
  console.log(`Main: Spawning engine at ${enginePath}`);

  // Standard editor session parameters
  // Detached mode: Engine runs in its own window, no shared texture.
  engineProcess = spawn(enginePath, ['-mode', '2', '-renderer', '0', '-loglevel', '1', '-parent_pid', process.pid.toString()], {
    cwd: binDir
  });

  engineProcess.stdout.on('data', (data) => {
    console.log(`Engine: ${data}`);
  });

  engineProcess.stderr.on('data', (data) => {
    console.error(`Engine Error: ${data}`);
  });

  engineProcess.on('exit', (code) => {
    console.log(`Main: Engine exited with code ${code}`);
    engineProcess = null;
  });

  // Connect to Engine WebSocket
  setTimeout(connectToEngine, 2000); // Wait for engine to start
}

function stopEngine() {
  if (engineProcess) {
    console.log('Main: Stopping engine...');
    engineProcess.kill();
    engineProcess = null;
  }
}

function restartEngine() {
  console.log('Main: Restarting engine...');
  stopEngine();
  setTimeout(spawnEngine, 1000);
}

function createWindow() {
  // Use absolute path for icon
  const iconPath = path.resolve(__dirname, '../../Data/EngineAssets/icon.png');
  console.log(`Main: Loading application icon from ${iconPath}`);
  
  const icon = nativeImage.createFromPath(iconPath);

  win = new BrowserWindow({ 
    width: 1600, 
    height: 900,
    icon: icon,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });
  
  // Disable native menu bar in favor of Naive UI menu
  Menu.setApplicationMenu(null);

  // Load the app - check if we are in production (dist) or dev
  const indexPath = path.join(__dirname, 'dist/index.html');
  if (require('fs').existsSync(indexPath)) {
    win.loadFile(indexPath);
  } else {
    // In dev mode, we need to load from the Vite dev server instead of raw index.html
    win.loadURL('http://localhost:5173');
  }

  win.webContents.on('console-message', (event, level, message, line, sourceId) => {
    console.log(`[RENDERER] ${message}`);
  });

  spawnEngine();
}

function connectToEngine() {
  console.log('Main: Connecting to Engine WS...');
  socket = new WebSocket('ws://127.0.0.1:8081');

  socket.on('open', () => {
    console.log('Main: Connected to Engine');
    socket.send(JSON.stringify({ type: 'HELO', pid: process.pid }));
    if (win) win.webContents.send('engine-connected', true);
  });

  socket.on('message', (data) => {
    const msg = JSON.parse(data);
    console.log('Main: Message from Engine:', msg);

    // Proxy other messages to renderer
    if (win) win.webContents.send('engine-message', msg);
  });

  socket.on('close', () => {
    console.log('Main: Disconnected from Engine');
    if (win) win.webContents.send('engine-connected', false);
    setTimeout(connectToEngine, 2000);
  });
}

app.whenReady().then(() => {
  createWindow();
  
  ipcMain.on('engine-stop', () => {
    stopEngine();
  });

  ipcMain.on('engine-restart', () => {
    restartEngine();
  });

  ipcMain.on('engine-message', (event, msg) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify(msg));
    }
  });

  ipcMain.on('select-files', async (event) => {
    const { dialog } = require('electron');
    const result = await dialog.showOpenDialog(win, {
      title: 'Select 3D Models to Import',
      properties: ['openFile', 'multiSelections'],
      filters: [
        { name: '3D Models', extensions: ['obj', 'fbx', 'gltf', 'glb'] }
      ]
    });

    if (!result.canceled && result.filePaths.length > 0) {
      event.reply('files-selected', result.filePaths);
    }
  });
});

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
