const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');
const fs = require('fs');

// The engine publishes DevToggleRegistry over IPC and the panel renders
// a row per toggle / action. PT + Screenshot are the two
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

    // Engine registers PT (toggle) and Screenshot (action) in
    // ExampleRenderingClient::Setup. Both must show up in the panel.
    await window.waitForSelector('[data-test="toggle-row-PT"]', { timeout: 10000 });
    await window.waitForSelector('[data-test="action-btn-Screenshot"]', { timeout: 10000 });

    // Toggle the path tracer on, then verify the ENGINE's own getter
    // reports it on — not just the optimistic UI flip. setToggle now
    // commits the engine's read-back (SET_DEV_TOGGLE replies with the
    // actual state after its setter ran), and a subsequent refresh()
    // re-queries the engine, so an assertion against
    // __innoStores.devToggle after that round-trip is genuinely server
    // truth.
    await window.waitForFunction(() => !!window.__innoStores?.devToggle, { timeout: 5000 });
    const ptSwitch = window.locator('[data-test="toggle-switch-PT"]');
    await ptSwitch.click();
    await expect(ptSwitch).toHaveAttribute('aria-checked', 'true');

    // Force a fresh LIST_DEV_TOGGLES round-trip and assert the value
    // the engine reports for PT is true.
    const reported = await window.evaluate(async () => {
      await window.__innoStores.devToggle.refresh();
      const t = window.__innoStores.devToggle.toggles.find((x) => x.name === 'PT');
      return t ? t.value : null;
    });
    expect(reported, 'engine-reported PT state after set').toBe(true);

    // Screenshot action: the optimistic "Screenshot triggered" toast is
    // gone — the panel now waits for the engine's SCREENSHOT_SAVED event
    // (broadcast by EditorService after the rendering client finishes
    // the save) and shows a result toast naming the absolute saved path.
    // Engine launches in -dump_frames-style timing so the per-frame save
    // happens within a couple of frames of the click; allow a generous
    // window for slow-scene cases.
    await window.click('[data-test="action-btn-Screenshot"]');
    const successToast = window.locator('.n-message:has-text("Screenshot saved:")');
    await successToast.waitFor({ timeout: 30000 });
    const toastText = (await successToast.innerText()).trim();
    const match = toastText.match(/Screenshot saved:\s+(.+\.(?:png|hdr))\s*$/);
    expect(match, `toast should name an absolute .png/.hdr path; got: ${toastText}`).not.toBeNull();
    const savedPath = match[1].trim();
    expect(
      fs.existsSync(savedPath),
      `saved screenshot should exist on disk at ${savedPath}`,
    ).toBe(true);
  } finally {
    await electronApp.close().catch(() => {});
  }
});
