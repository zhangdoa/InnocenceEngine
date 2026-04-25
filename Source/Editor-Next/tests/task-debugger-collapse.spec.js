/**
 * TASK-92 contract spec: TaskDebuggerPanel renders one compact row per
 * worker thread by default, expands to show the per-task detail on click,
 * and persists the toggle state across reload via uiStore.
 *
 * Follows the inline-mock pattern from scene-vertical.spec.js — no live
 * engine needed. We synthesize a LIST_TASKS reply with N threads and drive
 * the panel from the renderer side, which gives deterministic assertions
 * for thread count + expand/collapse state.
 */

const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

const N_THREADS = 4;

function fakeReport(name, startUs, durationUs) {
  return {
    name,
    startTime: String(startUs),
    finishTime: String(startUs + durationUs),
  };
}

function fakeThreads(count) {
  // Each thread gets a few synthetic reports with predictable durations so
  // the workload bar has something to scale against.
  const baseStart = 1_000_000_000; // microseconds
  const threads = [];
  for (let i = 0; i < count; ++i) {
    const reports = [];
    // Vary count and duration per thread so totals differ visibly.
    for (let k = 0; k < 3 + i; ++k) {
      reports.push(fakeReport(
        `Task_${i}_${k}`,
        baseStart + i * 10_000 + k * 200,
        100 + i * 25 + k * 10, // µs
      ));
    }
    threads.push({ index: i, reports });
  }
  return threads;
}

async function launchEditor() {
  const app = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });
  const window = await app.firstWindow();
  await window.waitForSelector('.editor-shell', { timeout: 15000 });
  await window.waitForFunction(() => !!window.__innoIpc, { timeout: 5000 });
  await window.waitForFunction(() => !!window.__innoStores?.ui, { timeout: 5000 });
  return { app, window };
}

async function installEngineMock(window, threads) {
  await window.evaluate((threadsPayload) => {
    const { ipcRenderer } = window.require('electron');
    const originalSend = ipcRenderer.send.bind(ipcRenderer);
    const responders = new Map();

    responders.set('GET_SCENE',           () => ({ entities: [] }));
    responders.set('LIST_DEV_TOGGLES',    () => ({ toggles: [], actions: [] }));
    responders.set('LIST_RENDER_TARGETS', () => ({ passes: [], override: null }));
    responders.set('LIST_TASKS',          () => ({ threads: threadsPayload }));

    ipcRenderer.send = (channel, msg) => {
      if (channel !== 'engine-message' || !msg || msg.envelope !== 'request') {
        return originalSend(channel, msg);
      }
      const fn = responders.get(msg.type);
      setTimeout(() => {
        if (!fn) {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: msg.id, status: 'err',
            error: { code: 'NO_HANDLER', message: msg.type },
          });
          return;
        }
        ipcRenderer.emit('engine-message', {}, {
          envelope: 'reply', id: msg.id, status: 'ok', result: fn(msg.payload ?? {}),
        });
      }, 5);
    };

    ipcRenderer.emit('connection-status', {}, {
      status: 'live', attempt: 0, error: null, nextRetryMs: null,
    });
    ipcRenderer.emit('engine-connected', {}, true);
  }, threads);
}

test.describe('TaskDebuggerPanel — collapse/expand UX (TASK-92)', () => {
  test.beforeEach(async ({}, testInfo) => {
    testInfo.setTimeout(60_000);
  });

  test('renders one compact row per worker thread by default', async () => {
    const { app, window } = await launchEditor();
    try {
      // Clear any persisted expansion state from prior runs so "default" is
      // genuinely the default.
      await window.evaluate(() => {
        try { localStorage.removeItem('editor-next.taskDebugger.expanded'); } catch {}
        if (window.__innoStores?.ui) {
          window.__innoStores.ui.taskDebuggerExpanded = {};
        }
      });

      await installEngineMock(window, fakeThreads(N_THREADS));

      await window.waitForSelector('[data-test="task-debugger-panel"]', { state: 'attached', timeout: 10_000 });
      // Wait for the LIST_TASKS reply to populate the store. The panel
      // polls every 500ms, so this resolves quickly.
      await window.waitForFunction(
        (n) => document.querySelectorAll('[data-test^="task-thread-row-"]').length === n,
        N_THREADS,
        { timeout: 10_000 },
      );

      const rowCount = await window.evaluate(() =>
        document.querySelectorAll('[data-test^="task-thread-row-"]').length,
      );
      expect(rowCount).toBe(N_THREADS);

      // No detail widget should be visible by default.
      const detailCount = await window.evaluate(() =>
        document.querySelectorAll('[data-test^="task-thread-detail-"]').length,
      );
      expect(detailCount).toBe(0);

      // Compact rows should be roughly 24px tall (allow ±4px for borders).
      const heights = await window.evaluate(() =>
        Array.from(document.querySelectorAll('[data-test^="task-thread-row-"]'))
          .map((el) => Math.round(el.getBoundingClientRect().height)),
      );
      for (const h of heights) {
        expect(h, `row height ${h}`).toBeGreaterThanOrEqual(20);
        expect(h, `row height ${h}`).toBeLessThanOrEqual(32);
      }

      // Workload metric (ms) should be visible on each row.
      const totals = await window.evaluate(() =>
        Array.from(document.querySelectorAll('[data-test^="task-thread-row-"] .thread-total'))
          .map((el) => el.textContent.trim()),
      );
      expect(totals).toHaveLength(N_THREADS);
      for (const t of totals) expect(t).toMatch(/^\d+\.\d{2} ms$/);
    } finally {
      await app.close().catch(() => {});
    }
  });

  test('clicking a row toggles the detail widget', async () => {
    const { app, window } = await launchEditor();
    try {
      await window.evaluate(() => {
        try { localStorage.removeItem('editor-next.taskDebugger.expanded'); } catch {}
        if (window.__innoStores?.ui) {
          window.__innoStores.ui.taskDebuggerExpanded = {};
        }
      });

      await installEngineMock(window, fakeThreads(N_THREADS));
      await window.waitForFunction(
        (n) => document.querySelectorAll('[data-test^="task-thread-row-"]').length === n,
        N_THREADS,
        { timeout: 10_000 },
      );

      // Toggle thread 1 open.
      await window.click('[data-test="task-thread-row-1"]');
      await window.waitForSelector('[data-test="task-thread-detail-1"]', { timeout: 2_000 });

      // Other threads remain collapsed.
      const otherDetails = await window.evaluate(() =>
        Array.from(document.querySelectorAll('[data-test^="task-thread-detail-"]'))
          .map((el) => el.getAttribute('data-test')),
      );
      expect(otherDetails).toEqual(['task-thread-detail-1']);

      // The detail widget shows per-task report rows.
      const reportNames = await window.evaluate(() =>
        Array.from(document.querySelectorAll('[data-test="task-thread-detail-1"] .report-name'))
          .map((el) => el.textContent.trim()),
      );
      expect(reportNames.length).toBeGreaterThan(0);
      for (const name of reportNames) expect(name).toMatch(/^Task_1_\d+$/);

      // Click again to collapse.
      await window.click('[data-test="task-thread-row-1"]');
      await window.waitForFunction(
        () => !document.querySelector('[data-test="task-thread-detail-1"]'),
        null,
        { timeout: 2_000 },
      );

      // aria-expanded reflects the toggle.
      const expandedAttr = await window.evaluate(() =>
        document.querySelector('[data-test="task-thread-row-1"]').getAttribute('data-expanded'),
      );
      expect(expandedAttr).toBe('false');
    } finally {
      await app.close().catch(() => {});
    }
  });

  test('expanded state persists across editor reload', async () => {
    const { app, window } = await launchEditor();
    try {
      // Hermetic start: drop any stale persisted state.
      await window.evaluate(() => {
        try { localStorage.removeItem('editor-next.taskDebugger.expanded'); } catch {}
        if (window.__innoStores?.ui) {
          window.__innoStores.ui.taskDebuggerExpanded = {};
        }
      });

      await installEngineMock(window, fakeThreads(N_THREADS));
      await window.waitForFunction(
        (n) => document.querySelectorAll('[data-test^="task-thread-row-"]').length === n,
        N_THREADS,
        { timeout: 10_000 },
      );

      // Expand thread 2 and confirm localStorage was written.
      await window.click('[data-test="task-thread-row-2"]');
      await window.waitForSelector('[data-test="task-thread-detail-2"]', { timeout: 2_000 });
      // The watcher on uiStore.taskDebuggerExpanded is async; give it a tick
      // to flush before reading.
      await window.waitForFunction(() => {
        const raw = localStorage.getItem('editor-next.taskDebugger.expanded');
        if (!raw) return false;
        try { return JSON.parse(raw)['2'] === true; } catch { return false; }
      }, null, { timeout: 2_000 });

      // Reload the renderer — userData (and therefore localStorage) survives
      // a renderer reload, so the next boot of uiStore.js should hydrate
      // from the persisted JSON.
      await window.reload();
      await window.waitForSelector('.editor-shell', { timeout: 15_000 });
      await window.waitForFunction(() => !!window.__innoStores?.ui, { timeout: 5_000 });

      // Sanity: persisted entry survived the reload.
      const persistedAtBoot = await window.evaluate(() =>
        localStorage.getItem('editor-next.taskDebugger.expanded'),
      );
      expect(persistedAtBoot).toBeTruthy();
      expect(JSON.parse(persistedAtBoot)['2']).toBe(true);

      // uiStore hydrated from localStorage at import time.
      const expandedFromStore = await window.evaluate(() =>
        !!window.__innoStores?.ui?.taskDebuggerExpanded?.['2'],
      );
      expect(expandedFromStore).toBe(true);

      // Re-install the engine mock for the post-reload renderer (the IPC
      // override doesn't survive reload — the renderer module graph was
      // rebuilt) and assert the detail widget reappears without a click.
      await installEngineMock(window, fakeThreads(N_THREADS));
      await window.waitForFunction(
        (n) => document.querySelectorAll('[data-test^="task-thread-row-"]').length === n,
        N_THREADS,
        { timeout: 10_000 },
      );
      await window.waitForSelector('[data-test="task-thread-detail-2"]', { timeout: 5_000 });

      // Cleanup so the next test in this file (or the next CI run sharing
      // userData) starts from collapsed default.
      await window.evaluate(() => {
        try { localStorage.removeItem('editor-next.taskDebugger.expanded'); } catch {}
      });
    } finally {
      await app.close().catch(() => {});
    }
  });
});
