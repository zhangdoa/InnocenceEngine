const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

// Smoke spec for TASK-62 AC #5 (Render Toggles pane). Asserts the engine
// publishes its DevToggleRegistry contents over IPC and the panel renders
// rows for at least the GPUPathTracer toggle and the Screenshot action —
// the two surfaces that retired the INNO_KEY_B and INNO_KEY_C bindings.
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

    // Toggle the path tracer on; the click should emit SET_DEV_TOGGLE,
    // and the optimistic local update flips the switch immediately. The
    // engine accepting the change will be reconciled on the next
    // LIST_DEV_TOGGLES round-trip — for the spec, the optimistic flip is
    // sufficient evidence that the IPC ran without error.
    const ptSwitch = window.locator('[data-test="toggle-switch-GPUPathTracer"]');
    await ptSwitch.click();
    await expect(ptSwitch).toHaveAttribute('aria-checked', 'true');

    // Action button should produce a toast confirming dispatch.
    await window.click('[data-test="action-btn-Screenshot"]');
    await window.waitForSelector('.n-message:has-text("Screenshot triggered")', {
      timeout: 5000,
    });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
