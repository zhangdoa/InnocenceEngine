const { app, BrowserWindow, sharedTexture, ipcMain, Menu, nativeImage } = require('electron');
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
  engineProcess = spawn(enginePath, ['-mode', '2', '-renderer', '0', '-loglevel', '0', '-parent_pid', process.pid.toString()], {
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
  const iconPath = path.resolve(__dirname, '../../Data/Engine/Icons/icon.png');
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
  
  // Disable native menu bar
  Menu.setApplicationMenu(null);

  // Load the app
  const indexPath = path.join(__dirname, 'dist/index.html');
  if (require('fs').existsSync(indexPath)) {
    win.loadFile(indexPath);
  } else {
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

    if (msg.type === 'HELLO_REPLY' || msg.type === 'VIEWPORT_READY') {
      if (msg.sharedHandle && msg.sharedHandle !== 0) {
        setupSharedTexture(msg);
      }
    }

    // Proxy other messages to renderer
    if (win) win.webContents.send('engine-message', msg);
  });

  socket.on('close', () => {
    console.log('Main: Disconnected from Engine');
    if (win) win.webContents.send('engine-connected', false);
    if (engineProcess) {
      setTimeout(connectToEngine, 5000);
    }
  });
}

function setupSharedTexture(info) {
  if (importedTexture) {
    importedTexture.release();
  }

  console.log('Main: sharedTexture API:', Object.keys(sharedTexture));
  if (sharedTexture.subtle) {
    console.log('Main: sharedTexture.subtle API:', Object.keys(sharedTexture.subtle));
  }
  console.log(`Main: Importing Shared Texture 0x${info.sharedHandle.toString(16)} (${info.width}x${info.height})`);

  try {
    console.log(`Main: Importing Shared Texture 0x${info.sharedHandle.toString(16)} (${info.width}x${info.height})`);

    importedTexture = sharedTexture.importSharedTexture({
      source: {
        type: 'd3d12-shared-handle',
        handle: BigInt(info.sharedHandle)
      },
      width: info.width,
      height: info.height,
      format: info.format === 'rgba' ? 'rgba8' : 'bgra8'
    });

    // Send to renderer
    sharedTexture.sendSharedTexture({
      frame: win.webContents.mainFrame,
      importedSharedTexture: importedTexture
    });

    console.log('Main: Shared texture sent to renderer');
  } catch (e) {
    console.error('Main: Failed to import shared texture:', e);
    
    // Fallback: try the previous structure but with different format names
    try {
      console.log('Main: Trying fallback structure...');
      importedTexture = sharedTexture.importSharedTexture({
        textureInfo: {
          handle: BigInt(info.sharedHandle),
          pixelFormat: info.format === 'rgba' ? 'rgba8unorm' : 'bgra8unorm',
          codedSize: { width: info.width, height: info.height },
          visibleRect: { x: 0, y: 0, width: info.width, height: info.height }
        }
      });
      
      sharedTexture.sendSharedTexture({
        frame: win.webContents.mainFrame,
        importedSharedTexture: importedTexture
      });
      console.log('Main: Fallback import successful');
    } catch (e2) {
      console.error('Main: Fallback also failed:', e2);
    }
  }
}

app.whenReady().then(() => {
  createWindow();
  
  ipcMain.on('engine-stop', () => stopEngine());
  ipcMain.on('engine-restart', () => restartEngine());
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
      filters: [{ name: '3D Models', extensions: ['obj', 'fbx', 'gltf', 'glb'] }]
    });

    if (!result.canceled && result.filePaths.length > 0) {
      win.webContents.send('files-selected', result.filePaths);
    }
  });
});

app.on('window-all-closed', () => {
  if (engineProcess) engineProcess.kill();
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
