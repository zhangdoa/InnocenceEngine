/**
 * Connection lifecycle state machine (TASK-88).
 *
 * main.js is the authority for the state machine and emits
 * `connection-status` payloads; the renderer's connectionStore mirrors
 * them. These tests simulate main.js by firing `connection-status` events
 * on ipcRenderer — same shape main would send — and assert the store +
 * domain fan-out + UI all react correctly.
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
  await window.waitForFunction(() => !!window.__innoStores?.connection, { timeout: 5000 });
  return { app, window };
}

function fire(window, status, extra = {}) {
  return window.evaluate(([status, extra]) => {
    const { ipcRenderer } = window.require('electron');
    ipcRenderer.emit('connection-status', {}, {
      status,
      attempt: extra.attempt ?? 0,
      error: extra.error ?? null,
      nextRetryMs: extra.nextRetryMs ?? null,
    });
    // Keep the binary signal in sync so useIpc's engine-connected bus
    // subscribers fan out to domain stores.
    ipcRenderer.emit('engine-connected', {}, status === 'live');
  }, [status, extra]);
}

test('status transitions flow through connectionStore', async () => {
  const { app, window } = await launchEditor();
  try {
    await fire(window, 'connecting', { attempt: 0 });
    await fire(window, 'live');
    const liveState = await window.evaluate(() => {
      const c = window.__innoStores.connection;
      return { status: c.status, isConnected: c.isConnected };
    });
    expect(liveState.status).toBe('live');
    expect(liveState.isConnected).toBe(true);

    await fire(window, 'lost', { nextRetryMs: 2000 });
    const lostState = await window.evaluate(() => {
      const c = window.__innoStores.connection;
      return { status: c.status, isConnected: c.isConnected, msg: c.lastMessage };
    });
    expect(lostState.status).toBe('lost');
    expect(lostState.isConnected).toBe(false);
    expect(lostState.msg).toContain('Connection lost');

    await fire(window, 'giving-up', { error: 'tcp: connection refused' });
    const giveUpState = await window.evaluate(() => {
      const c = window.__innoStores.connection;
      return { status: c.status, msg: c.lastMessage };
    });
    expect(giveUpState.status).toBe('giving-up');
    expect(giveUpState.msg).toContain('connection refused');
  } finally {
    await app.close().catch(() => {});
  }
});

test('every domain store has onConnect/onDisconnect and reacts on transitions', async () => {
  const { app, window } = await launchEditor();
  try {
    const methodShape = await window.evaluate(() => {
      const s = window.__innoStores;
      const report = {};
      for (const key of ['scene', 'asset', 'devToggle', 'renderTarget', 'taskGraph']) {
        report[key] = {
          hasOnConnect:    typeof s[key].onConnect === 'function',
          hasOnDisconnect: typeof s[key].onDisconnect === 'function',
        };
      }
      return report;
    });
    for (const [name, r] of Object.entries(methodShape)) {
      expect(r.hasOnConnect,    `${name}.onConnect`).toBe(true);
      expect(r.hasOnDisconnect, `${name}.onDisconnect`).toBe(true);
    }

    // Pre-populate some state, then force disconnect: every store resets.
    await window.evaluate(() => {
      const s = window.__innoStores;
      s.scene.entities = [{ id: 1, name: 'dummy' }];
      s.scene.selectedEntity = { id: 1, components: [] };
      s.asset.isImporting = true;
      s.devToggle.toggles = [{ name: 't', value: true }];
      s.renderTarget.passes = [{ name: 'p', targets: [] }];
      s.taskGraph.threads = [{ index: 0, reports: [] }];
    });

    await fire(window, 'lost', { nextRetryMs: 2000 });

    const cleared = await window.evaluate(() => {
      const s = window.__innoStores;
      return {
        sceneEntities: s.scene.entities.length,
        sceneSelected: s.scene.selectedEntity,
        assetImporting: s.asset.isImporting,
        devToggles: s.devToggle.toggles.length,
        rtPasses: s.renderTarget.passes.length,
        tgThreads: s.taskGraph.threads.length,
      };
    });
    expect(cleared).toEqual({
      sceneEntities: 0,
      sceneSelected: null,
      assetImporting: false,
      devToggles: 0,
      rtPasses: 0,
      tgThreads: 0,
    });
  } finally {
    await app.close().catch(() => {});
  }
});

test('footer renders retry button only in giving-up state', async () => {
  const { app, window } = await launchEditor();
  try {
    await fire(window, 'live');
    await window.waitForTimeout(50);
    let visible = await window.isVisible('[data-test="connection-retry"]').catch(() => false);
    expect(visible).toBe(false);

    await fire(window, 'giving-up', { error: 'unreachable' });
    await window.waitForSelector('[data-test="connection-retry"]', { timeout: 2000 });
    visible = await window.isVisible('[data-test="connection-retry"]');
    expect(visible).toBe(true);

    // Clicking retry sends engine-retry on the ipcRenderer main channel; we
    // intercept outgoing sends to prove the wire up.
    const sent = await window.evaluate(async () => {
      const { ipcRenderer } = window.require('electron');
      const captured = [];
      const orig = ipcRenderer.send.bind(ipcRenderer);
      ipcRenderer.send = (ch, msg) => {
        captured.push({ ch, msg });
        return orig(ch, msg);
      };
      document.querySelector('[data-test="connection-retry"]').click();
      await new Promise(r => setTimeout(r, 50));
      ipcRenderer.send = orig;
      return captured.map(c => c.ch);
    });
    expect(sent).toContain('engine-retry');
  } finally {
    await app.close().catch(() => {});
  }
});
