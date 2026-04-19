const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// Window submenu lists every registered panel; toggling closes it in
// dockview and toggling again re-opens it.
test('window menu toggles panels', async () => {
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
    await window.waitForSelector('.dock-container', { state: 'attached', timeout: 30000 });

    // The Hierarchy / Outliner pane is the easiest non-viewport panel to
    // assert on (its CSS class is unique).
    await window.waitForSelector('.hierarchy-panel', { state: 'attached', timeout: 15000 });

    // Open the Window menu, click the Outliner toggle to close it.
    await window.click('.menu-bar >> text=Window');
    await window.waitForSelector('[data-test="window-toggle-hierarchy_panel"]', {
      timeout: 5000,
    });
    await window.click('[data-test="window-toggle-hierarchy_panel"]');

    // Outliner should be gone from the dock.
    await expect(window.locator('.hierarchy-panel')).toHaveCount(0, { timeout: 5000 });

    // Open Window menu again and re-toggle to bring it back.
    await window.click('.menu-bar >> text=Window');
    await window.click('[data-test="window-toggle-hierarchy_panel"]');
    await window.waitForSelector('.hierarchy-panel', { state: 'attached', timeout: 5000 });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
