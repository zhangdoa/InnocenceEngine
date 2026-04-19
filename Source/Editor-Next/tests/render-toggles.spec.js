const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// The engine publishes DevToggleRegistry over IPC and the panel renders
// a row per toggle / action. GPUPathTracer + Screenshot are the two
// entries the example rendering client always registers.
test('render toggles pane lists engine-registered toggles + actions', async () => {
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
    await window.waitForSelector('[data-test="render-toggles-panel"]', {
      state: 'attached',
      timeout: 15000,
    });

    // Engine registers GPUPathTracer (toggle) and Screenshot (action) in
    // ExampleRenderingClient::Setup. Both must show up in the panel.
    await window.waitForSelector('[data-test="toggle-row-GPUPathTracer"]', { timeout: 10000 });
    await window.waitForSelector('[data-test="action-btn-Screenshot"]', { timeout: 10000 });

    // Toggle the path tracer on, then verify the ENGINE's own getter
    // reports it on — not just the optimistic UI flip. setToggle now
    // commits the engine's read-back (SET_DEV_TOGGLE replies with the
    // actual state after its setter ran), and a subsequent refresh()
    // re-queries the engine, so an assertion against
    // __innoStores.devToggle after that round-trip is genuinely server
    // truth.
    await window.waitForFunction(() => !!window.__innoStores?.devToggle, { timeout: 5000 });
    const ptSwitch = window.locator('[data-test="toggle-switch-GPUPathTracer"]');
    await ptSwitch.click();
    await expect(ptSwitch).toHaveAttribute('aria-checked', 'true');

    // Force a fresh LIST_DEV_TOGGLES round-trip and assert the value
    // the engine reports for GPUPathTracer is true.
    const reported = await window.evaluate(async () => {
      await window.__innoStores.devToggle.refresh();
      const t = window.__innoStores.devToggle.toggles.find((x) => x.name === 'GPUPathTracer');
      return t ? t.value : null;
    });
    expect(reported, 'engine-reported GPUPathTracer state after set').toBe(true);

    // Action button should produce a toast confirming dispatch.
    await window.click('[data-test="action-btn-Screenshot"]');
    await window.waitForSelector('.n-message:has-text("Screenshot triggered")', {
      timeout: 5000,
    });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
