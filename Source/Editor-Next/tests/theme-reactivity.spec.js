/**
 * Regression for TASK-91: panels visibly follow theme changes.
 *
 * What this guards against:
 *   dockview-core's default theme fallback is `themeAbyss` — if no `theme`
 *   prop is passed to `<dockview-vue>`, dockview-core applies
 *   `dockview-theme-abyss` as a class on an internal node, and THAT class's
 *   `--dv-*` tokens shadow any theme class on our outer wrapper. Result:
 *   the Dockview chrome stays abyss-coloured in every flavor. AppLayout
 *   passes `:theme="ctpTheme"` to fix this; this spec catches anyone who
 *   removes it.
 *
 * Flavor switches go through `uiStore.setTheme` directly (not the menu UI)
 * because the menu-click sequence is flaky under back-to-back automated
 * flips.
 */

const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

const FLAVOR_MANTLE_RGB = {
  latte:     'rgb(230, 233, 239)',  // #e6e9ef
  frappe:    'rgb(41, 44, 60)',     // #292c3c
  macchiato: 'rgb(30, 32, 48)',     // #1e2030
  mocha:     'rgb(24, 24, 37)',     // #181825
}
const FLAVOR_BASE_RGB = {
  latte:     'rgb(239, 241, 245)',  // #eff1f5
  frappe:    'rgb(48, 52, 70)',     // #303446
  macchiato: 'rgb(36, 39, 58)',     // #24273a
  mocha:     'rgb(30, 30, 46)',     // #1e1e2e
}

test('panels and dockview chrome repaint on theme flavor change', async () => {
  test.setTimeout(60000);
  const electronApp = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });

  try {
    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 15000 });
    await window.waitForFunction(() => !!window.__innoStores?.ui, { timeout: 5000 });
    // Wait for dockview to finish mounting its first tab bar.
    await window.waitForSelector('.dv-tabs-and-actions-container', { timeout: 5000 });

    const flip = async (flavor) => {
      await window.evaluate((f) => window.__innoStores.ui.setTheme(f), flavor);
      await window.waitForTimeout(200);
    };

    const readBg = (sel) => window.evaluate((s) => {
      const el = document.querySelector(s);
      return el ? getComputedStyle(el).backgroundColor : null;
    }, sel);

    for (const flavor of ['latte', 'mocha', 'frappe', 'macchiato']) {
      await flip(flavor);

      const htmlClass = await window.evaluate(() => document.documentElement.className);
      expect(htmlClass, `html class after flip to ${flavor}`).toContain(`ctp-${flavor}`);

      const bodyBg = await readBg('body');
      expect(bodyBg, `body bg in ${flavor}`).toBe(FLAVOR_BASE_RGB[flavor]);

      const footerBg = await readBg('.editor-footer');
      expect(footerBg, `footer bg in ${flavor}`).toBe(FLAVOR_MANTLE_RGB[flavor]);

      // Guards against dropping the :theme prop on <dockview-vue>: if
      // the prop is missing, dockview-core falls back to themeAbyss on
      // its inner node and this bg locks to #1c1c2a across all flavors.
      const tabBg = await readBg('.dv-tabs-and-actions-container');
      expect(tabBg, `dockview tab bar bg in ${flavor}`).toBe(FLAVOR_MANTLE_RGB[flavor]);

      // Guards against dropping ThemedPanelHost: dockview-vue's
      // `mountVueComponent` only merges direct-parent provides when
      // mounting a panel, so App.vue's outer NConfigProvider theme
      // doesn't reach NInput inside the panel. Without the host
      // wrapper, Naive falls back to its default theme (white bg in
      // both mocha and latte, green focus rings). We read --n-color
      // directly off NInput's host — that's the variable Naive
      // generates from themeOverrides.
      const inputColor = await window.evaluate(() => {
        const el = document.querySelector('.hierarchy-panel .n-input');
        return el ? getComputedStyle(el).getPropertyValue('--n-color').trim() : null;
      });
      const mantleHex = FLAVOR_MANTLE_RGB[flavor].replace(/rgb\((\d+), (\d+), (\d+)\)/, (_, r, g, b) =>
        '#' + [r, g, b].map((x) => Number(x).toString(16).padStart(2, '0')).join('')
      );
      expect(inputColor?.toLowerCase(), `n-input --n-color in ${flavor}`).toBe(mantleHex);
    }
  } finally {
    await electronApp.close().catch(() => {});
  }
});
