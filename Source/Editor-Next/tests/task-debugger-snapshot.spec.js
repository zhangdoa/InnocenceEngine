/**
 * One-shot snapshot helper for TASK-92 deliverable. Not part of the regression
 * suite — just produces a DOM dump under Build/captures/ for the closure note.
 *
 * Run: npx playwright test tests/task-debugger-snapshot.spec.js --workers=1
 */

const { _electron: electron } = require('@playwright/test');
const { test } = require('@playwright/test');
const path = require('path');
const fs = require('fs');

test('task-92 deliverable: capture collapsed-default + one-expanded DOM snapshot', async () => {
  test.setTimeout(60_000);
  const app = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });
  const window = await app.firstWindow();
  await window.waitForSelector('.editor-shell', { timeout: 15000 });
  await window.waitForFunction(() => !!window.__innoStores?.ui, { timeout: 5000 });

  await window.evaluate(() => {
    try { localStorage.removeItem('editor-next.taskDebugger.expanded'); } catch {}
    if (window.__innoStores?.ui) window.__innoStores.ui.taskDebuggerExpanded = {};
    const { ipcRenderer } = window.require('electron');
    const responders = new Map();
    const fakeReports = (i) => Array.from({ length: 3 + i }, (_, k) => ({
      name: `Task_${i}_${k}`,
      startTime: String(1_000_000_000 + i * 10_000 + k * 200),
      finishTime: String(1_000_000_000 + i * 10_000 + k * 200 + 100 + i * 25 + k * 10),
    }));
    const threads = Array.from({ length: 4 }, (_, i) => ({ index: i, reports: fakeReports(i) }));
    responders.set('LIST_TASKS',          () => ({ threads }));
    responders.set('LIST_DEV_TOGGLES',    () => ({ toggles: [], actions: [] }));
    responders.set('LIST_RENDER_TARGETS', () => ({ passes: [], override: null }));
    responders.set('GET_SCENE',           () => ({ entities: [] }));
    const orig = ipcRenderer.send.bind(ipcRenderer);
    ipcRenderer.send = (ch, m) => {
      if (ch !== 'engine-message' || m?.envelope !== 'request') return orig(ch, m);
      const fn = responders.get(m.type);
      setTimeout(() => {
        if (!fn) {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: m.id, status: 'err',
            error: { code: 'NO_HANDLER', message: m.type } });
          return;
        }
        ipcRenderer.emit('engine-message', {}, {
          envelope: 'reply', id: m.id, status: 'ok', result: fn(m.payload ?? {}) });
      }, 5);
    };
    ipcRenderer.emit('connection-status', {}, { status: 'live', attempt: 0, error: null, nextRetryMs: null });
    ipcRenderer.emit('engine-connected', {}, true);
  });

  await window.waitForFunction(
    () => document.querySelectorAll('[data-test^="task-thread-row-"]').length === 4,
    null, { timeout: 10_000 },
  );

  const captureDir = path.join(__dirname, '..', '..', '..', 'Build', 'captures', 'task-92');
  fs.mkdirSync(captureDir, { recursive: true });

  const collapsedHtml = await window.evaluate(() =>
    document.querySelector('[data-test="task-thread-list"]').outerHTML);
  fs.writeFileSync(path.join(captureDir, 'collapsed-default.html'), collapsedHtml);

  await window.click('[data-test="task-thread-row-1"]');
  await window.waitForSelector('[data-test="task-thread-detail-1"]', { timeout: 2_000 });

  const expandedHtml = await window.evaluate(() =>
    document.querySelector('[data-test="task-thread-list"]').outerHTML);
  fs.writeFileSync(path.join(captureDir, 'thread-1-expanded.html'), expandedHtml);

  // Also drop a tiny screenshot of the panel for visual reference.
  const panelHandle = await window.$('[data-test="task-debugger-panel"]');
  if (panelHandle) {
    await panelHandle.screenshot({ path: path.join(captureDir, 'panel-expanded.png') });
  }

  await app.close().catch(() => {});
});
