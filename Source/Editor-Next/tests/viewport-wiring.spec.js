/**
 * Viewport wiring (TASK-89). main.js is the source of VIEWPORT_READY /
 * VIEWPORT_FAILED — these tests mock main's emission by firing the same
 * wire-shaped events on ipcRenderer and assert ViewportPanel reacts.
 */

const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

async function launchEditor() {
  const app = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });
  const window = await app.firstWindow();
  await window.waitForSelector('.editor-shell', { timeout: 15000 });
  await window.waitForSelector('[data-test="viewport-panel"]', { timeout: 5000 });
  return { app, window };
}

function emitEngineEvent(window, type, payload) {
  return window.evaluate(([type, payload]) => {
    const { ipcRenderer } = window.require('electron');
    ipcRenderer.emit('engine-message', {}, {
      envelope: 'event',
      type,
      payload,
    });
  }, [type, payload]);
}

test('VIEWPORT_READY transitions viewport from pending → live with correct dims', async () => {
  const { app, window } = await launchEditor();
  try {
    // Initially pending.
    const initial = await window.getAttribute('[data-test="viewport-panel"]', 'data-test-status');
    expect(initial).toBe('pending');

    await emitEngineEvent(window, 'VIEWPORT_READY', { width: 1920, height: 1080 });
    await window.waitForSelector('[data-test="viewport-panel"][data-test-status="live"]', { timeout: 2000 });

    const res = await window.textContent('[data-test="viewport-resolution"]');
    expect(res.trim()).toBe('1920 × 1080');
  } finally {
    await app.close().catch(() => {});
  }
});

test('VIEWPORT_FAILED shows explicit error state with retry button', async () => {
  const { app, window } = await launchEditor();
  try {
    await emitEngineEvent(window, 'VIEWPORT_FAILED', { reason: 'shared-texture bind failed' });
    await window.waitForSelector('[data-test="viewport-panel"][data-test-status="failed"]', { timeout: 2000 });
    await window.waitForSelector('[data-test="viewport-retry"]', { timeout: 1000 });

    const body = await window.textContent('.overlay');
    expect(body).toContain('shared-texture bind failed');

    // Clicking retry flips back to pending and sends engine-retry upstream.
    const outgoing = await window.evaluate(async () => {
      const { ipcRenderer } = window.require('electron');
      const captured = [];
      const orig = ipcRenderer.send.bind(ipcRenderer);
      ipcRenderer.send = (ch, msg) => { captured.push({ ch, msg }); return orig(ch, msg); };
      document.querySelector('[data-test="viewport-retry"]').click();
      await new Promise(r => setTimeout(r, 80));
      ipcRenderer.send = orig;
      return captured.map(c => c.ch);
    });
    expect(outgoing).toContain('engine-retry');

    await window.waitForSelector('[data-test="viewport-panel"][data-test-status="pending"]', { timeout: 1000 });
  } finally {
    await app.close().catch(() => {});
  }
});

test('disconnect returns viewport to pending regardless of prior state', async () => {
  const { app, window } = await launchEditor();
  try {
    await emitEngineEvent(window, 'VIEWPORT_READY', { width: 800, height: 600 });
    await window.waitForSelector('[data-test="viewport-panel"][data-test-status="live"]', { timeout: 2000 });

    await window.evaluate(() => {
      const { ipcRenderer } = window.require('electron');
      ipcRenderer.emit('engine-connected', {}, false);
    });

    await window.waitForSelector('[data-test="viewport-panel"][data-test-status="pending"]', { timeout: 2000 });
  } finally {
    await app.close().catch(() => {});
  }
});
