const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// Engine streams TaskScheduler reports over IPC; the panel renders one
// recent-task list per worker thread with proportional duration bars.
test('task debugger panel populates from engine reports', async () => {
  test.setTimeout(180000);
  const electronApp = await electron.launch({
    args: [
      '.',
      '--engine=Main',
      '--no-sandbox',
      '--disable-gpu',
      '--disable-dev-shm-usage',
    ],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });

  try {
    electronApp.on('console', (msg) => console.log(`[ELECTRON] ${msg.text()}`));

    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 30000 });
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });
    await window.waitForSelector('[data-test="task-debugger-panel"]', {
      state: 'attached',
      timeout: 15000,
    });

    // Auto-refresh polls every 500ms, so reports populate quickly. Assert
    // at least one Thread N header appears (engine always has worker
    // threads alive once it's Live).
    await window.waitForSelector('.task-debugger >> text=Thread 0', { timeout: 15000 });

    // Refresh button still works when auto is off.
    await window.click('[data-test="task-auto"]');
    await window.click('[data-test="task-refresh"]');
  } finally {
    await electronApp.close().catch(() => {});
  }
});
