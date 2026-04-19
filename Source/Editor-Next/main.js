const { app, BrowserWindow, ipcMain, Menu, nativeImage } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const WebSocket = require('ws');

let engineProcess;
let win;
let socket;

// ──────────────────────────────────────────────────────────────────────────
// Connection state machine
//
//   idle → connecting → live → lost → connecting … → giving-up
//                                 ↓
//   stopping ← (user action from any state)
//
// Every transition fires a `connection-status` event to the renderer carrying
// { status, attempt, error? }. A derived `engine-connected` boolean is fired
// alongside for the parts of useIpc / domain stores that treat connection as
// a binary (they can migrate to status later).
// ──────────────────────────────────────────────────────────────────────────
const RECONNECT_DELAYS_MS = [2000, 4000, 8000, 16000, 30000];
const GIVE_UP_AFTER_ATTEMPTS = 6; // ≥ RECONNECT_DELAYS_MS.length

let connStatus = 'idle';
let connAttempt = 0;
let connLastError = null;
let reconnectTimer = null;
let userInitiatedShutdown = false;

function getReconnectDelay(attempt) {
  const capIndex = RECONNECT_DELAYS_MS.length - 1;
  return RECONNECT_DELAYS_MS[Math.min(attempt, capIndex)];
}

function setConnStatus(status, extra = {}) {
  connStatus = status;
  if (extra.attempt !== undefined) connAttempt = extra.attempt;
  if (extra.error !== undefined)   connLastError = extra.error;
  if (!win) return;
  const payload = {
    status: connStatus,
    attempt: connAttempt,
    error: connLastError,
    nextRetryMs: status === 'lost' ? getReconnectDelay(connAttempt) : null,
  };
  win.webContents.send('connection-status', payload);
  // Binary derived signal: everything that pre-dates phase 4 treats 'live'
  // as the only "connected" state.
  win.webContents.send('engine-connected', status === 'live');
}

function cancelReconnect() {
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
}

function scheduleReconnect() {
  cancelReconnect();
  if (userInitiatedShutdown) return;
  if (connAttempt >= GIVE_UP_AFTER_ATTEMPTS) {
    setConnStatus('giving-up');
    return;
  }
  const delay = getReconnectDelay(connAttempt);
  console.log(`Main: Reconnect attempt ${connAttempt + 1} in ${delay}ms`);
  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connAttempt += 1;
    connectToEngine();
  }, delay);
}

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

  engineProcess = spawn(enginePath, ['-mode', '2', '-renderer', '0', '-loglevel', '0', '-parent_pid', process.pid.toString()], {
    cwd: binDir
  });

  engineProcess.stdout.on('data', (data) => { console.log(`Engine: ${data}`); });
  engineProcess.stderr.on('data', (data) => { console.error(`Engine Error: ${data}`); });
  engineProcess.on('exit', (code) => {
    console.log(`Main: Engine exited with code ${code}`);
    engineProcess = null;
  });

  // Wait for engine to bind its WS server, then initiate the connection.
  setTimeout(connectToEngine, 2000);
}

function stopEngine() {
  userInitiatedShutdown = true;
  cancelReconnect();
  setConnStatus('stopping');
  if (socket) {
    try { socket.close(); } catch {}
    socket = null;
  }
  if (engineProcess) {
    console.log('Main: Stopping engine...');
    engineProcess.kill();
    engineProcess = null;
  }
  setConnStatus('idle', { attempt: 0, error: null });
}

function restartEngine() {
  console.log('Main: Restarting engine...');
  stopEngine();
  userInitiatedShutdown = false;
  setTimeout(spawnEngine, 1000);
}

/** Manual retry from the renderer (e.g. footer "Retry" button in giving-up). */
function retryNow() {
  console.log('Main: Manual retry requested.');
  cancelReconnect();
  userInitiatedShutdown = false;
  setConnStatus('connecting', { attempt: 0, error: null });
  if (!engineProcess) {
    spawnEngine();
  } else {
    connectToEngine();
  }
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

  // Push initial status so the renderer shows "connecting…" from the first paint.
  win.webContents.once('did-finish-load', () => {
    setConnStatus(connStatus);
  });

  win.webContents.on('console-message', (event, level, message, line, sourceId) => {
    console.log(`[RENDERER] ${message}`);
  });

  spawnEngine();
}

// HELLO is a liveness handshake: main sends it on socket open; the engine
// replies, confirming it's ready to accept requests. The reply carries no
// client-consumable payload — the editor does not embed the engine's
// viewport (see project CLAUDE.md: the game view lives in Main.exe's own
// OS window, not inside the Electron frame). Use id=0 so renderer-
// generated ids (useIpc's nextRequestId starts at 1) never collide.
const HELLO_REQUEST_ID = 0;

function connectToEngine() {
  if (connStatus !== 'connecting') setConnStatus('connecting');
  console.log('Main: Connecting to Engine WS...');
  try {
    socket = new WebSocket('ws://127.0.0.1:8081');
  } catch (e) {
    console.error('Main: WebSocket construction failed:', e?.message || e);
    setConnStatus('lost', { error: String(e?.message || e) });
    scheduleReconnect();
    return;
  }

  socket.on('open', () => {
    console.log('Main: Connected to Engine');
    cancelReconnect();
    connAttempt = 0;
    connLastError = null;
    setConnStatus('live', { attempt: 0, error: null });
    try {
      socket.send(JSON.stringify({
        envelope: 'request',
        id: HELLO_REQUEST_ID,
        type: 'HELLO',
        payload: {},
      }));
    } catch (e) {
      console.error('Main: HELLO send failed:', e?.message || e);
    }
  });

  socket.on('error', (err) => {
    // Don't transition state here — 'error' usually precedes 'close', which
    // owns the state transition. Just record the error message.
    connLastError = err?.message || String(err);
    console.error('Main: WebSocket error:', connLastError);
  });

  socket.on('message', (data) => {
    let msg;
    try { msg = JSON.parse(data); }
    catch (e) { console.error('Main: failed to parse engine message:', e); return; }

    // HELLO reply — main owns it (renderer never issues HELLO). Log outcome
    // and drop; the reply is only used as an aliveness signal, the setConnStatus
    // 'live' transition already happened on socket 'open'.
    if (msg.envelope === 'reply' && msg.id === HELLO_REQUEST_ID) {
      if (msg.status === 'err') {
        console.error('Main: HELLO rejected by engine:', msg.error);
      }
      return;
    }

    if (win) win.webContents.send('engine-message', msg);
  });

  socket.on('close', () => {
    console.log('Main: Disconnected from Engine');
    socket = null;
    if (userInitiatedShutdown) {
      // A stop/restart already drove the state machine to 'stopping'/'idle'.
      return;
    }
    setConnStatus('lost');
    scheduleReconnect();
  });
}

app.whenReady().then(() => {
  createWindow();

  ipcMain.on('engine-stop', () => stopEngine());
  ipcMain.on('engine-restart', () => restartEngine());
  ipcMain.on('engine-retry', () => retryNow());
  ipcMain.on('engine-message', (event, msg) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify(msg));
    }
  });

  // Renderer asks the main process for the Data/ directory because the
  // renderer bundle loses __dirname (Vite flat-bundles ESM, where it is
  // undefined). Resolved here from main.js's stable __dirname anchor.
  ipcMain.handle('get-data-dir', () => path.resolve(__dirname, '../../Data'));

  ipcMain.on('select-files', async (event) => {
    const { dialog } = require('electron');
    const result = await dialog.showOpenDialog(win, {
      title: 'Select assets to import',
      properties: ['openFile', 'multiSelections'],
      filters: [
        { name: '3D models', extensions: ['obj', 'fbx', 'gltf', 'glb', 'ply', 'md5mesh'] },
        { name: 'Textures',  extensions: ['png', 'jpg', 'jpeg', 'tga'] },
        { name: 'All files', extensions: ['*'] },
      ]
    });

    if (!result.canceled && result.filePaths.length > 0) {
      win.webContents.send('files-selected', result.filePaths);
    }
  });

  ipcMain.on('select-folder', async () => {
    const { dialog } = require('electron');
    const fs = require('fs');
    const nodePath = require('path');
    const result = await dialog.showOpenDialog(win, {
      title: 'Select a folder to bulk-import (every supported file inside)',
      properties: ['openDirectory'],
    });
    if (result.canceled || !result.filePaths.length) return;

    const folder = result.filePaths[0];
    const entries = fs.readdirSync(folder).filter((name) =>
      /\.(png|jpe?g|tga|obj|fbx|gltf|glb|ply|md5mesh)$/i.test(name)
    );
    const filePaths = entries.map((name) => nodePath.join(folder, name));
    if (filePaths.length > 0) {
      win.webContents.send('files-selected', filePaths);
    }
  });
});

app.on('window-all-closed', () => {
  cancelReconnect();
  if (engineProcess) engineProcess.kill();
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
