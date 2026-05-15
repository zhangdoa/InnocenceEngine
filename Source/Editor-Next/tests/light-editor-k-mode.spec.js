const { test, expect } = require('@playwright/test');
const {
  launchAgainstEngine,
  findLightEntity,
  readEngineLight,
} = require('./helpers/light-editor-fixture');

test('LightEditor K-mode toggle round-trips through the Use Temp. checkbox', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    const target = await findLightEntity(window);
    expect(target).not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, target.id);
    await window.waitForSelector('.n-form-item:has-text("Use Temp.")', { timeout: 5000 });
    await window.waitForSelector('.n-form-item:has-text("Temperature")', { timeout: 5000 });

    const initial = await readEngineLight(window, target.id);
    expect(initial.useColorTemperature, 'GISponza lights ship with K-mode true').toBe(true);

    await window.evaluate(() => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const row = labels.find(l => l.textContent.trim() === 'Use Temp.');
      const item = row.closest('.n-form-item');
      const checkbox = item.querySelector('.n-checkbox');
      checkbox.click();
    });

    const afterOff = await window.evaluate(async (id) => {
      for (let i = 0; i < 30; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light && light.useColorTemperature === false) return light;
        await new Promise(res => setTimeout(res, 100));
      }
      return null;
    }, target.id);
    expect(afterOff, 'engine useColorTemperature after first click').not.toBeNull();
    expect(afterOff.useColorTemperature).toBe(false);

    // Temperature input must be disabled when K-mode is off — the gating is
    // the discoverability promise of the K-mode surface (K input only edits
    // the value when K-mode actually owns the color).
    const tempDisabledOff = await window.evaluate(() => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const row = labels.find(l => l.textContent.trim() === 'Temperature');
      const item = row.closest('.n-form-item');
      const input = item.querySelector('input');
      return input ? input.disabled : null;
    });
    expect(tempDisabledOff, 'Temperature input disabled when K-mode off').toBe(true);

    await window.evaluate(() => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const row = labels.find(l => l.textContent.trim() === 'Use Temp.');
      const item = row.closest('.n-form-item');
      const checkbox = item.querySelector('.n-checkbox');
      checkbox.click();
    });

    const afterOn = await window.evaluate(async (id) => {
      for (let i = 0; i < 30; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light && light.useColorTemperature === true) return light;
        await new Promise(res => setTimeout(res, 100));
      }
      return null;
    }, target.id);
    expect(afterOn, 'engine useColorTemperature after second click').not.toBeNull();
    expect(afterOn.useColorTemperature).toBe(true);
  } finally {
    await app.close().catch(() => {});
  }
});

test('LightEditor Temperature input round-trips and re-derives RGB when K-mode is on', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    const target = await findLightEntity(window);
    expect(target).not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, target.id);
    await window.waitForSelector('.n-form-item:has-text("Temperature")', { timeout: 5000 });

    const initial = await readEngineLight(window, target.id);
    expect(initial.useColorTemperature, 'GISponza lights ship with K-mode true').toBe(true);
    const initialColor = [...initial.color];

    const newK = 6500;
    await window.evaluate((val) => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const row = labels.find(l => l.textContent.trim() === 'Temperature');
      const item = row.closest('.n-form-item');
      const input = item.querySelector('input');
      input.focus();
      input.value = String(val);
      input.dispatchEvent(new Event('input',  { bubbles: true }));
      input.dispatchEvent(new Event('change', { bubbles: true }));
      input.blur();
    }, newK);

    // The K write is synchronous in the IPC handler, but the K→RGB
    // re-derivation runs on the simulation service's frame. Poll until BOTH
    // K is set AND RGB has visibly shifted from initial — otherwise the loop
    // can exit on the first frame where K=6500 but m_RGBColor is still the
    // K=5780 cache.
    const afterK = await window.evaluate(async ({ id, expK, init, tol }) => {
      const colorChanged = (c) =>
        Math.abs(c[0] - init[0]) > tol ||
        Math.abs(c[1] - init[1]) > tol ||
        Math.abs(c[2] - init[2]) > tol;
      for (let i = 0; i < 60; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light && Math.abs(light.colorTemperature - expK) < tol && colorChanged(light.color)) {
          return light;
        }
        await new Promise(res => setTimeout(res, 100));
      }
      return null;
    }, { id: target.id, expK: newK, init: initialColor, tol: 1e-3 });

    expect(afterK, 'engine colorTemperature + RGB re-derive after K input edit').not.toBeNull();
    expect(Math.abs(afterK.colorTemperature - newK)).toBeLessThan(1e-3);
    const colorChanged =
      Math.abs(afterK.color[0] - initialColor[0]) > 1e-3 ||
      Math.abs(afterK.color[1] - initialColor[1]) > 1e-3 ||
      Math.abs(afterK.color[2] - initialColor[2]) > 1e-3;
    expect(colorChanged, 'RGB color must re-derive from new K when K-mode is on').toBe(true);
  } finally {
    await app.close().catch(() => {});
  }
});
