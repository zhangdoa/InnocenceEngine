/**
 * TASK-204 regression: every dockview-mounted panel must propagate
 * `height:100%` from `.dv-content-container` through ThemedPanelHost's
 * n-config-provider div down to the panel root. Without this, the panel
 * root falls back to intrinsic content height, the inner n-scrollbar
 * measures itself the same as its content (scrollHeight==clientHeight,
 * scroll never activates), and overflow gets clipped by .dv-groupview's
 * `overflow:hidden` — forcing the user to resize the panel to reveal
 * hidden options.
 *
 * The fix: `style="height: 100%"` on `<n-config-provider>` in
 * ThemedPanelHost.vue. This spec asserts both the structural shape
 * (height chain) and the user-observable outcome (overflow is
 * scrollable, hidden buttons become reachable without resize).
 */
const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

async function launchEditor() {
  return electron.launch({
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
}

test('TASK-204 — ThemedPanelHost propagates height through every dockview-mounted panel', async () => {
  test.setTimeout(240000);
  const electronApp = await launchEditor();

  try {
    electronApp.on('console', (m) => console.log(`[ELECTRON] ${m.text()}`));
    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 30000 });
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });

    // --- Part A: RenderTargetDebuggerPanel — exemplar called out by user.
    await window.waitForSelector('[data-test="render-target-debugger-panel"]', {
      state: 'attached', timeout: 15000,
    });
    await window.waitForSelector('[data-test="render-target-refresh"]', {
      state: 'attached', timeout: 15000,
    });

    const rtSizing = await window.evaluate(() => {
      const panel = document.querySelector('[data-test="render-target-debugger-panel"]');
      const dvContent = panel?.closest('.dv-content-container');
      // The n-config-provider wrapper rendered by ThemedPanelHost sits
      // between dvContent and the panel root. It carries the inline
      // `height:100%` that this fix adds.
      const configProvider = dvContent?.firstElementChild;
      const container = panel?.querySelector('.n-scrollbar-container');
      return {
        dvContentClient: dvContent?.clientHeight,
        configProviderClient: configProvider?.clientHeight,
        configProviderTag: configProvider?.tagName.toLowerCase(),
        panelClient: panel?.clientHeight,
        scrollContainerClient: container?.clientHeight,
        scrollContainerScroll: container?.scrollHeight,
      };
    });
    console.log('[TASK-204] RT-debugger sizing:', JSON.stringify(rtSizing));

    // Structural assertion: every link in the chain matches its parent's
    // client height. dv-content-container -> ThemedPanelHost wrapper ->
    // panel root -> n-scrollbar-container.
    expect(rtSizing.configProviderClient,
      'ThemedPanelHost wrapper must fill dv-content-container height').toBe(rtSizing.dvContentClient);
    expect(rtSizing.panelClient,
      'panel root must fill ThemedPanelHost wrapper height').toBe(rtSizing.dvContentClient);
    expect(rtSizing.scrollContainerClient,
      'n-scrollbar-container must fill panel height (else scroll never activates)').toBe(rtSizing.dvContentClient);

    // User-observable outcome assertion. Only meaningful when the engine
    // reports enough render targets that picker-row + buttons exceed the
    // panel cell. RT count is engine-determined and not deterministic in
    // tests, so guard on actual overflow.
    if (rtSizing.scrollContainerScroll > rtSizing.scrollContainerClient) {
      const scrollResult = await window.evaluate(() => {
        const c = document.querySelector('[data-test="render-target-debugger-panel"] .n-scrollbar-container');
        const before = c.scrollTop;
        c.scrollTop = 9999;
        const after = c.scrollTop;
        return { before, after, moved: after > before };
      });
      console.log('[TASK-204] RT-debugger scroll result:', JSON.stringify(scrollResult));
      expect(scrollResult.moved,
        'overflowing scroll-container must move scrollTop on user scroll').toBe(true);

      const reachable = await window.evaluate(() => {
        const groupView = document.querySelector('[data-test="render-target-debugger-panel"]')?.closest('.dv-groupview');
        const apply = document.querySelector('[data-test="render-target-apply"]');
        if (!groupView || !apply) return null;
        return {
          groupBottom: groupView.getBoundingClientRect().bottom,
          applyBottom: apply.getBoundingClientRect().bottom,
          applyVisible: apply.getBoundingClientRect().bottom <= groupView.getBoundingClientRect().bottom,
        };
      });
      console.log('[TASK-204] RT-debugger Apply reachable post-scroll:', JSON.stringify(reachable));
      expect(reachable.applyVisible,
        'Apply button must sit inside groupView clip area after user scroll').toBe(true);
    } else {
      console.log('[TASK-204] RT-debugger content fits in panel cell; no scroll needed');
    }

    // --- Part B: PropertyPanel — confirm fix generalizes to a second panel.
    // Load GISponza so a LightEntity exists, populating the inspector with
    // many form fields (the predecessor's repro2 confirmed PropertyPanel +
    // LightEditor naturally overflows the 420px cell).
    await window.waitForFunction(() => !!window.__innoIpc, { timeout: 5000 });
    await window.waitForFunction(() => !!window.__innoStores?.scene, { timeout: 5000 });

    await window.evaluate(async () => {
      const { request, on } = window.__innoIpc;
      const sceneUpdated = new Promise((resolve) => {
        const off = on('SCENE_UPDATED', () => { off(); resolve(); });
      });
      await request('LOAD_SCENE', { path: 'ExampleProject/Scenes/GISponza.InnoScene' }, { timeoutMs: 60000 });
      await sceneUpdated;
    });

    const targetId = await window.evaluate(async () => {
      const { request } = window.__innoIpc;
      const { entities } = await request('GET_SCENE');
      for (const e of entities) {
        const r = await request('GET_ENTITY_DETAILS', { id: e.id });
        const has = (r.details.components || []).some((c) => c.type === 'LightComponent');
        if (has) return e.id;
      }
      return null;
    });
    expect(targetId, 'GISponza must contain at least one LightComponent entity').not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, targetId);
    await window.waitForSelector('.n-form-item:has-text("Cast Shadow")', { timeout: 5000 });

    const propSizing = await window.evaluate(() => {
      const panel = document.querySelector('.property-panel');
      const dvContent = panel?.closest('.dv-content-container');
      const configProvider = dvContent?.firstElementChild;
      return {
        dvContentClient: dvContent?.clientHeight,
        configProviderClient: configProvider?.clientHeight,
        panelClient: panel?.clientHeight,
      };
    });
    console.log('[TASK-204] PropertyPanel sizing:', JSON.stringify(propSizing));

    expect(propSizing.configProviderClient,
      'ThemedPanelHost wrapper must fill dv-content-container height (PropertyPanel)').toBe(propSizing.dvContentClient);
    expect(propSizing.panelClient,
      'PropertyPanel root must fill ThemedPanelHost wrapper height').toBe(propSizing.dvContentClient);

    // PropertyPanel uses its own outer flex column (no n-scrollbar at the
    // root); when it overflows, the inner sections scroll. The structural
    // assertion above is sufficient — without the fix, panel.clientHeight
    // would equal its intrinsic content height (>> dvContent), so the
    // assertion alone catches the regression.

    await window.screenshot({
      path: path.join(__dirname, '..', '..', '..', 'Build', 'captures', 'task-204-after-fix.png'),
      fullPage: false,
    });
  } finally {
    await electronApp.close().catch(() => {});
  }
});
