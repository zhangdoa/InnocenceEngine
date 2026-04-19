const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// Regression spec for the AssetPanel scene-load path after the bug fixes:
//  - baseDir resolved from __dirname (was process.cwd() — wrong outside the
//    editor source dir; Playwright launches from `tests/` cwd, exactly the
//    case that broke before).
//  - load-scene listener properly removed on unmount (was a leaked
//    anonymous handler).
//  - LOAD_SCENE dispatch goes through useIpc.js, which now toasts and
//    short-circuits when the engine is offline.
test('asset panel double-click on .InnoScene loads the scene', async () => {
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

    // Asset panel mounts under id 'assets_panel' in AppLayout.
    await window.waitForSelector('.asset-panel', { state: 'attached', timeout: 15000 });

    // Drill into ExampleProject → Scenes via double-click on folders.
    await window.locator('.asset-item:has-text("ExampleProject")').dblclick();
    await window.locator('.asset-item:has-text("Scenes")').dblclick();

    // The .InnoScene rows should now be visible. Double-click GISponza.
    await window.waitForSelector('.asset-item:has-text("GISponza.InnoScene")', { timeout: 10000 });
    await window.locator('.asset-item:has-text("GISponza.InnoScene")').dblclick();

    // useIpc.js shows an info toast when LOAD_SCENE fires; absence here would
    // mean the load-scene event never reached the listener.
    await window.waitForSelector('.n-message:has-text("Loading")', { timeout: 10000 });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
