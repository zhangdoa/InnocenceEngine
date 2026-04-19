const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// Smoke spec for TASK-62 AC #2 (Scene Picker pane). Asserts the panel
// enumerates *.InnoScene files from disk and dispatches LOAD_SCENE on click.
// Replaces the old `R → UnitTest` / `L → GISponza` keyboard handlers — those
// are deleted from World.inl, so this spec is now the regression gate for
// scene loading.

test('scene picker lists scenes and loads on click', async () => {
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
    electronApp.on('console', (msg) => {
      console.log(`[ELECTRON] ${msg.text()}`);
    });

    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 30000 });
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });

    // Scene panel is registered in AppLayout under id 'scenes_panel'.
    await window.waitForSelector('[data-test="scene-panel"]', { state: 'attached', timeout: 15000 });

    // Expect at minimum the three top-level scenes shipped under
    // Data/ExampleProject/Scenes/. Any missing row means the fs enumeration
    // is broken or the data dir was relocated.
    const expected = ['GISponza', 'GITestBox', 'UnitTest'];
    for (const name of expected) {
      await window.waitForSelector(`[data-test="scene-row-${name}"]`, { timeout: 10000 });
    }

    // Click GISponza; assert it goes "active" (engine ack arrives via toast +
    // the row gets the highlight icon). The deeper SCENE_DATA round-trip is
    // already covered by the editor smoke spec; here we just need the click
    // to emit the IPC.
    await window.click('[data-test="scene-row-GISponza"]');
    await window.waitForSelector('.n-message:has-text("Loading GISponza")', { timeout: 10000 });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
