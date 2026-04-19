const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

test('render-target-debugger lists passes and routes a chosen RT to the viewport', async () => {
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
    await window.waitForSelector('[data-test="render-target-debugger-panel"]', {
      state: 'attached',
      timeout: 15000,
    });

    // Picker should populate from LIST_RENDER_TARGETS once the engine
    // enumerates passes (frame 1 onward).
    await window.waitForSelector('[data-test="render-target-picker"]', { timeout: 15000 });

    const picker = window.locator('[data-test="render-target-picker"]');
    await picker.click();
    const firstOption = window.locator('.n-base-select-option').first();
    await firstOption.waitFor({ timeout: 10000 });
    const optionLabel = await firstOption.innerText();
    await firstOption.click();

    await window.click('[data-test="render-target-apply"]');
    await window.waitForSelector(`.n-message:has-text("Viewport now showing")`, {
      timeout: 10000,
    });

    await window.click('[data-test="render-target-reset"]');
    await window.waitForSelector('.n-message:has-text("Viewport reset")', { timeout: 10000 });

    // Sanity: the chosen label was non-empty so the IPC carried something.
    expect(optionLabel.length).toBeGreaterThan(0);
  } finally {
    await electronApp.close().catch(() => {});
  }
});
