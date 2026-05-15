const { test, expect } = require('@playwright/test');
const { launchAgainstEngine } = require('./helpers/light-editor-fixture');

test('RenderTogglesPanel flips the engine state for RasterizedGI via the switch click', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    await window.waitForSelector('[data-test="render-toggles-panel"]', {
      state: 'attached', timeout: 15000,
    });
    await window.waitForSelector('[data-test="toggle-row-RasterizedGI"]', { timeout: 10000 });

    const initial = await window.evaluate(async () => {
      await window.__innoStores.devToggle.refresh();
      return window.__innoStores.devToggle.toggles.find(t => t.name === 'RasterizedGI').value;
    });

    const sw = window.locator('[data-test="toggle-switch-RasterizedGI"]');
    await sw.click();
    await expect(sw).toHaveAttribute('aria-checked', String(!initial));

    const reported = await window.evaluate(async () => {
      await window.__innoStores.devToggle.refresh();
      return window.__innoStores.devToggle.toggles.find(t => t.name === 'RasterizedGI').value;
    });
    expect(reported, 'engine-reported RasterizedGI state after click').toBe(!initial);
  } finally {
    await app.close().catch(() => {});
  }
});
