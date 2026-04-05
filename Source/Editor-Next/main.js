const { app, BrowserWindow, sharedTexture, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const WebSocket = require('ws');

let engineProcess;
let win;
let socket;
let importedTexture;

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
    win.loadFile('index.html');
  }
  
  // Determine engine path from args or default to RenderTest
  let engineExeName = 'RenderTest.exe';
  const engineArg = process.argv.find(arg => arg.startsWith('--engine='));
  if (engineArg) {
    engineExeName = engineArg.split('=')[1] + '.exe';
  }

  const enginePath = path.join(__dirname, '../../Bin/RelWithDebInfo/', engineExeName);
  console.log(`Main: Spawning engine at ${enginePath}`);
  
  engineProcess = spawn(enginePath, ['-sidecar', '-renderer 0', '-loglevel 0', '-offscreen', '-test draw_instanced']);

  engineProcess.stdout.on('data', (data) => {
    console.log(`Engine: ${data}`);
  });

  engineProcess.stderr.on('data', (data) => {
    console.error(`Engine Error: ${data}`);
  });

  // Connect to Engine WebSocket
  setTimeout(connectToEngine, 2000); // Wait for engine to start
}

function connectToEngine() {
  console.log('Main: Connecting to Engine WS...');
  socket = new WebSocket('ws://localhost:8081');

  socket.on('open', () => {
    console.log('Main: Connected to Engine');
    socket.send(JSON.stringify({ type: 'HELO' }));
  });

  socket.on('message', (data) => {
    const msg = JSON.parse(data);
    console.log('Main: Message from Engine:', msg);

    if (msg.type === 'HELLO_REPLY') {
      setupSharedTexture(msg);
    }
  });

  socket.on('close', () => {
    console.log('Main: Disconnected from Engine');
    setTimeout(connectToEngine, 2000);
  });
}

function setupSharedTexture(info) {
  if (importedTexture) {
    importedTexture.release();
  }

  console.log(`Main: Importing Shared Texture 0x${info.sharedHandle.toString(16)} (${info.width}x${info.height})`);

  try {
    // Note: handle might need to be a BigInt or a Buffer. 
    // sharedTexture.importSharedTexture expects a platform-specific handle object.
    // For Windows D3D12, it's an NT HANDLE.
    
    importedTexture = sharedTexture.importSharedTexture({
      textureInfo: {
        handle: BigInt(info.sharedHandle),
        pixelFormat: info.format === 'rgba' ? 'rgba' : 'bgra',
        codedSize: { width: info.width, height: info.height },
        visibleRect: { x: 0, y: 0, width: info.width, height: info.height }
      },
      allReferencesReleased: () => {
        console.log('Main: All references to shared texture released');
      }
    });

    // Send to renderer
    sharedTexture.sendSharedTexture({
      frame: win.webContents.mainFrame,
      importedSharedTexture: importedTexture
    });

    console.log('Main: Shared texture sent to renderer');
  } catch (e) {
    console.error('Main: Failed to import shared texture:', e);
  }
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
