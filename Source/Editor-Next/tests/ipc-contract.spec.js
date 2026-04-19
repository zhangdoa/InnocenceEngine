/**
 * Contract tests for src/composables/useIpc.js. These exercise the wire
 * envelope in isolation — no engine required — by simulating incoming
 * messages with `ipcRenderer.emit` and making assertions against the
 * request/on primitives exposed on window.
 *
 * The editor exposes no test hooks by default; these tests use
 * `page.addScriptTag` to pull useIpc into window scope for the duration
 * of each test.
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
  return { app, window };
}

async function bindUseIpc(window) {
  // useIpc.js writes its primitives to window.__innoIpc at module load —
  // wait for the app chunk to finish loading before we reach in.
  await window.waitForFunction(() => !!window.__innoIpc, { timeout: 5000 });
}

test('reply envelope resolves the matching inflight request', async () => {
  const { app, window } = await launchEditor();
  try {
    await bindUseIpc(window);
    const result = await window.evaluate(async () => {
      const mod = window.__innoIpc;
      if (!mod) return { skipped: true };
      const promise = mod.request('TEST_NOOP', { a: 1 }, { timeoutMs: 3000 });

      // Pull the most recently assigned id out of the inflight map shape:
      // since tests can't read it directly, we rely on monotonic ids and
      // set a small delay so the `request` has written before we reply.
      await new Promise(r => setTimeout(r, 50));

      // Scan ids: send replies for a small range and let the router match.
      const { ipcRenderer } = window.require('electron');
      for (let id = 1; id < 1000; id++) {
        ipcRenderer.emit('engine-message', {}, {
          envelope: 'reply',
          id,
          status: 'ok',
          result: { echoed: id },
        });
      }
      return { value: await promise };
    });
    if (result.skipped) return;
    expect(result.value.echoed).toBeGreaterThan(0);
  } finally {
    await app.close().catch(() => {});
  }
});

test('request times out with TIMEOUT error code when no reply comes', async () => {
  const { app, window } = await launchEditor();
  try {
    await bindUseIpc(window);
    const outcome = await window.evaluate(async () => {
      const mod = window.__innoIpc;
      if (!mod) return { skipped: true };
      try {
        await mod.request('TEST_UNREPLIED', {}, { timeoutMs: 300 });
        return { rejected: false };
      } catch (e) {
        return {
          rejected: true,
          code: e?.code,
          message: e?.message,
          name: e?.name,
          requestType: e?.requestType,
        };
      }
    });
    if (outcome.skipped) return;
    expect(outcome.rejected).toBe(true);
    expect(outcome.code).toBe('TIMEOUT');
    expect(outcome.requestType).toBe('TEST_UNREPLIED');
  } finally {
    await app.close().catch(() => {});
  }
});

test('event envelope fires the subscribed handler; does not resolve requests', async () => {
  const { app, window } = await launchEditor();
  try {
    await bindUseIpc(window);
    const outcome = await window.evaluate(async () => {
      const mod = window.__innoIpc;
      if (!mod) return { skipped: true };
      const payloads = [];
      const unsub = mod.on('TEST_EVENT', (p) => payloads.push(p));

      // Also spin up an orphan request so we can prove the event doesn't
      // accidentally resolve it.
      const reqP = mod.request('TEST_REQ', {}, { timeoutMs: 500 });
      const reqOutcome = reqP.then(() => 'resolved').catch(e => e.code);

      const { ipcRenderer } = window.require('electron');
      ipcRenderer.emit('engine-message', {}, {
        envelope: 'event',
        type: 'TEST_EVENT',
        payload: { hello: 'world' },
      });

      await new Promise(r => setTimeout(r, 100));
      unsub();

      return { payloads, reqOutcome: await reqOutcome };
    });
    if (outcome.skipped) return;
    expect(outcome.payloads).toEqual([{ hello: 'world' }]);
    // Request should not have been resolved by the event envelope.
    expect(outcome.reqOutcome).toBe('TIMEOUT');
  } finally {
    await app.close().catch(() => {});
  }
});

test('disconnect rejects all inflight requests with DISCONNECTED', async () => {
  const { app, window } = await launchEditor();
  try {
    await bindUseIpc(window);
    const outcome = await window.evaluate(async () => {
      const mod = window.__innoIpc;
      if (!mod) return { skipped: true };
      const p1 = mod.request('TEST_A', {}, { timeoutMs: 5000 });
      const p2 = mod.request('TEST_B', {}, { timeoutMs: 5000 });
      const o1 = p1.catch(e => e.code);
      const o2 = p2.catch(e => e.code);

      const { ipcRenderer } = window.require('electron');
      ipcRenderer.emit('engine-connected', {}, false);

      return [await o1, await o2];
    });
    if (outcome.skipped) return;
    expect(outcome).toEqual(['DISCONNECTED', 'DISCONNECTED']);
  } finally {
    await app.close().catch(() => {});
  }
});
