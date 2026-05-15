const { _electron: electron } = require('@playwright/test');
const path = require('path');

const FLOAT_TOL = 1e-4;

async function launchAgainstEngine() {
  const app = await electron.launch({
    args: [
      '.',
      '--engine=Main',
      '--no-sandbox',
      '--disable-gpu',
      '--disable-dev-shm-usage',
    ],
    cwd: path.join(__dirname, '..', '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });
  app.on('console', (m) => console.log(`[ELECTRON] ${m.text()}`));
  const window = await app.firstWindow();
  await window.waitForSelector('.editor-shell', { timeout: 30000 });
  await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });
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

  return { app, window };
}

async function findLightEntity(window) {
  return window.evaluate(async () => {
    const { request } = window.__innoIpc;
    const { entities } = await request('GET_SCENE');
    for (const e of entities) {
      const r = await request('GET_ENTITY_DETAILS', { id: e.id });
      const light = (r.details.components || []).find(c => c.type === 'LightComponent');
      if (light) {
        return {
          id: e.id, name: e.name,
          intensity: light.intensity, castShadow: light.castShadow, color: light.color,
        };
      }
    }
    return null;
  });
}

async function readEngineLight(window, id) {
  return window.evaluate(async (entId) => {
    const { request } = window.__innoIpc;
    const r = await request('GET_ENTITY_DETAILS', { id: entId });
    return (r.details.components || []).find(c => c.type === 'LightComponent') || null;
  }, id);
}

module.exports = { FLOAT_TOL, launchAgainstEngine, findLightEntity, readEngineLight };
