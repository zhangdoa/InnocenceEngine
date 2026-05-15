const { test, expect } = require('@playwright/test');
const {
  FLOAT_TOL,
  launchAgainstEngine,
  findLightEntity,
} = require('./helpers/light-editor-fixture');

test('LightEditor mounts on selection and round-trips intensity through the n-input-number', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    const target = await findLightEntity(window);
    expect(target, 'GISponza must contain at least one LightComponent entity').not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, target.id);
    await window.waitForSelector('.properties-content', { timeout: 5000 });
    await window.waitForSelector('.n-form-item:has-text("Cast Shadow")', { timeout: 5000 });

    const renderedIntensity = await window.evaluate(() => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const lum = labels.find(l => l.textContent.trim() === 'Luminous');
      if (!lum) return null;
      const item = lum.closest('.n-form-item');
      const input = item ? item.querySelector('input') : null;
      return input ? input.value : null;
    });
    expect(renderedIntensity, 'Luminous input mirrors engine intensity').not.toBeNull();
    expect(Math.abs(parseFloat(renderedIntensity) - target.intensity)).toBeLessThan(FLOAT_TOL);

    const newIntensity = target.intensity + 50;
    await window.evaluate((val) => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const lum = labels.find(l => l.textContent.trim() === 'Luminous');
      const item = lum.closest('.n-form-item');
      const input = item.querySelector('input');
      input.focus();
      input.value = String(val);
      input.dispatchEvent(new Event('input',  { bubbles: true }));
      input.dispatchEvent(new Event('change', { bubbles: true }));
      input.blur();
    }, newIntensity);

    const committed = await window.evaluate(async (id) => {
      for (let i = 0; i < 30; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light) return light.intensity;
        await new Promise(res => setTimeout(res, 100));
      }
      return null;
    }, target.id);
    expect(committed, 'engine intensity after Luminous edit').not.toBeNull();
    expect(Math.abs(committed - newIntensity)).toBeLessThan(FLOAT_TOL);
  } finally {
    await app.close().catch(() => {});
  }
});

test('LightEditor Cast Shadow checkbox round-trips through the n-checkbox click', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    const target = await findLightEntity(window);
    expect(target).not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, target.id);
    await window.waitForSelector('.n-form-item:has-text("Cast Shadow")', { timeout: 5000 });

    const initialCast = target.castShadow;
    await window.evaluate(() => {
      const labels = Array.from(document.querySelectorAll('.n-form-item-label'));
      const cs = labels.find(l => l.textContent.trim() === 'Cast Shadow');
      const item = cs.closest('.n-form-item');
      const checkbox = item.querySelector('.n-checkbox');
      checkbox.click();
    });

    const committedCast = await window.evaluate(async (id) => {
      for (let i = 0; i < 30; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light) return light.castShadow;
        await new Promise(res => setTimeout(res, 100));
      }
      return null;
    }, target.id);
    expect(committedCast, 'engine castShadow after click').toBe(!initialCast);
  } finally {
    await app.close().catch(() => {});
  }
});

test('LightEditor color picker round-trips through draft.color → commit', async () => {
  test.setTimeout(240000);
  const { app, window } = await launchAgainstEngine();
  try {
    const target = await findLightEntity(window);
    expect(target).not.toBeNull();

    await window.evaluate(async (id) => {
      await window.__innoStores.scene.selectEntity(id);
    }, target.id);
    await window.waitForSelector('.n-form-item:has-text("Color")', { timeout: 5000 });

    // n-color-picker's modal-driven commit path is canvas-rendered and teleported
    // to the body root; driving it through the popover is fragile. Invoke the
    // commit path directly so the spec exercises every layer after the picker
    // emits @update:value, which is the wiring a user-reported symptom would
    // actually break. Outer picker UX is covered by naive-ui's own suite.
    const newHex = '#ff8000';
    const expected = [1.0, 0x80 / 255, 0.0];
    await window.evaluate((hex) => {
      const r = parseInt(hex.slice(1, 3), 16) / 255;
      const g = parseInt(hex.slice(3, 5), 16) / 255;
      const b = parseInt(hex.slice(5, 7), 16) / 255;
      const id = window.__innoStores.scene.selectedEntity.id;
      return window.__innoStores.scene.updateProperty({
        id, component: 'LightComponent', property: 'color', value: [r, g, b],
      });
    }, newHex);

    const committedColor = await window.evaluate(async ({ id, exp, tol }) => {
      const close = (a, b) => Math.abs(a - b) < tol;
      for (let i = 0; i < 30; i++) {
        const { request } = window.__innoIpc;
        const r = await request('GET_ENTITY_DETAILS', { id });
        const light = (r.details.components || []).find(c => c.type === 'LightComponent');
        if (light && close(light.color[0], exp[0]) && close(light.color[1], exp[1]) && close(light.color[2], exp[2])) {
          return light.color;
        }
        await new Promise(res => setTimeout(res, 100));
      }
      const fallback = await window.__innoIpc.request('GET_ENTITY_DETAILS', { id });
      const lc = (fallback.details.components || []).find(c => c.type === 'LightComponent');
      return lc ? lc.color : null;
    }, { id: target.id, exp: expected, tol: FLOAT_TOL });

    expect(committedColor, 'engine color after commit').not.toBeNull();
    expect(Math.abs(committedColor[0] - expected[0])).toBeLessThan(FLOAT_TOL);
    expect(Math.abs(committedColor[1] - expected[1])).toBeLessThan(FLOAT_TOL);
    expect(Math.abs(committedColor[2] - expected[2])).toBeLessThan(FLOAT_TOL);
  } finally {
    await app.close().catch(() => {});
  }
});
