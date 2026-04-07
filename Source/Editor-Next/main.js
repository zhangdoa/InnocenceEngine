const { app, BrowserWindow, sharedTexture, ipcMain, Menu } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const WebSocket = require('ws');

let engineProcess;
let win;
let socket;
let importedTexture;

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

function createMenu() {
  const template = [
    {
      label: 'File',
      submenu: [
        { role: 'quit' }
      ]
    },
    {
      label: 'Engine',
      submenu: [
        {
          label: 'Restart Engine',
          accelerator: 'CmdOrCtrl+R',
          click: () => { restartEngine(); }
        },
        {
          label: 'Stop Engine',
          click: () => { stopEngine(); }
        }
      ]
    },
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'forceReload' },
        { role: 'toggleDevTools' },
        { type: 'separator' },
        { role: 'resetZoom' },
        { role: 'zoomIn' },
        { role: 'zoomOut' },
        { type: 'separator' },
        { role: 'togglefullscreen' }
      ]
    }
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
}

function createWindow() {
  win = new BrowserWindow({ 
    width: 1600, 
    height: 900,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });
  
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

  createMenu();
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

    if (msg.type === 'HELLO_REPLY') {
      if (msg.sharedHandle && msg.sharedHandle !== 0) {
        setupSharedTexture(msg);
      }
    } else if (msg.type === 'VIEWPORT_READY') {
      setupSharedTexture(msg);
      if (win) win.webContents.send('viewport-ready', msg);
    }
    
    // Proxy other messages to renderer if needed
    if (win) win.webContents.send('engine-message', msg);
  });

  socket.on('close', () => {
    console.log('Main: Disconnected from Engine');
    if (win) win.webContents.send('engine-connected', false);
    setTimeout(connectToEngine, 2000);
  });
}

function setupSharedTexture(info) {
  if (!info.sharedHandle || info.sharedHandle === 0) {
    console.log('Main: Skipping shared texture import (handle is 0)');
    return;
  }

  if (process.env.E2E_TEST) {
    console.log('Main: Skipping shared texture import due to E2E_TEST environment variable.');
    return;
  }

  if (importedTexture) {
    importedTexture.release();
  }

  const handleBigInt = BigInt(info.sharedHandle);
  console.log(`Main: Importing Shared Texture 0x${handleBigInt.toString(16).toUpperCase()} (${info.width}x${info.height})`);

  try {
    // Windows HANDLE is 64-bit on x64, convert BigInt to Little-Endian Buffer
    const handleBuffer = Buffer.alloc(8);
    handleBuffer.writeBigUInt64LE(handleBigInt, 0);

    importedTexture = sharedTexture.importSharedTexture({
      textureInfo: {
        handle: {
          ntHandle: handleBuffer
        },
        pixelFormat: info.format === 'rgba' ? 'rgba' : 'bgra',
        codedSize: { width: info.width, height: info.height },
        visibleRect: { x: 0, y: 0, width: info.width, height: info.height }
      },
      allReferencesReleased: () => {
        console.log('Main: All references to shared texture released');
      }
    });

    console.log('Main: Shared texture imported successfully. Waiting for renderer to request it...');
  } catch (e) {
    console.error('Main: Failed to import shared texture:', e);
  }
}

app.whenReady().then(() => {
  createWindow();
  
  ipcMain.on('renderer-ready-for-texture', () => {
    if (importedTexture && win) {
      console.log('Main: Renderer requested texture, sending now...');
      sharedTexture.sendSharedTexture({
        frame: win.webContents.mainFrame,
        importedSharedTexture: importedTexture
      }).then(() => {
        console.log('Main: Shared texture sent to renderer successfully');
      }).catch(e => {
        console.error('Main: Failed to send shared texture to renderer:', e.message);
      });
    } else {
      console.log('Main: Renderer requested texture, but none is imported yet.');
    }
  });

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
