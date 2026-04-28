/**
 * Visual-validation deliverable for the workspace-name-truncation +
 * outliner-duplicate-look fixes (user feedback 2026-04-28).
 *
 * Mock-IPC harness — fast deterministic UI verification. The live-engine
 * gate is satisfied separately by tests/scene-load.spec.js (workspace
 * double-click) which exercises the same components against a real engine.
 *
 * Run: npx playwright test tests/workspace-outliner-readability.spec.js --workers=1
 */

const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');
const fs = require('fs');

const captureDir = path.join(__dirname, '..', '..', '..', 'Build', 'captures', 'workspace-outliner-readability');
fs.mkdirSync(captureDir, { recursive: true });

async function launchWithMockedScene(entities) {
  const app = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  });
  const window = await app.firstWindow();
  await window.waitForSelector('.editor-shell', { timeout: 15000 });
  await window.waitForFunction(() => !!window.__innoStores?.ui, { timeout: 5000 });

  await window.evaluate((mockEntities) => {
    const { ipcRenderer } = window.require('electron');
    const responders = new Map();
    responders.set('LIST_TASKS',          () => ({ threads: [] }));
    responders.set('LIST_DEV_TOGGLES',    () => ({ toggles: [], actions: [] }));
    responders.set('LIST_RENDER_TARGETS', () => ({ passes: [], override: null }));
    responders.set('GET_SCENE',           () => ({ entities: mockEntities }));
    const orig = ipcRenderer.send.bind(ipcRenderer);
    ipcRenderer.send = (ch, m) => {
      if (ch !== 'engine-message' || m?.envelope !== 'request') return orig(ch, m);
      const fn = responders.get(m.type);
      setTimeout(() => {
        if (!fn) {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: m.id, status: 'err',
            error: { code: 'NO_HANDLER', message: m.type } });
          return;
        }
        ipcRenderer.emit('engine-message', {}, {
          envelope: 'reply', id: m.id, status: 'ok', result: fn(m.payload ?? {}) });
      }, 5);
    };
    ipcRenderer.emit('connection-status', {}, { status: 'live', attempt: 0, error: null, nextRetryMs: null });
    ipcRenderer.emit('engine-connected', {}, true);
  }, entities);

  return { app, window };
}

test('workspace asset names occupy the grid cell, not the icon-width', async () => {
  test.setTimeout(60_000);
  const { app, window } = await launchWithMockedScene([]);

  try {
    // The asset panel reads files from disk via the data-dir IPC; no scene
    // mocking is needed for this surface. Drill into a folder with longish
    // names so the truncation is visible.
    await window.waitForSelector('.asset-panel', { state: 'attached', timeout: 15000 });
    await window.locator('.asset-item:has-text("ExampleProject")').dblclick();
    await window.locator('.asset-item:has-text("Scenes")').dblclick();
    await window.waitForSelector('.asset-item:has-text("GISponza.InnoScene")', { timeout: 10_000 });

    // Measure the rendered width of an .asset-item against the parent grid
    // cell. Pre-fix the item shrink-wrapped to the icon (~52px); post-fix
    // it stretches to the cell width.
    const m = await window.evaluate(() => {
      const item = document.querySelector('.asset-item');
      const grid = document.querySelector('.asset-grid');
      if (!item || !grid) return null;
      const ir = item.getBoundingClientRect();
      const nameEl = item.querySelector('.asset-name');
      const nr = nameEl?.getBoundingClientRect();
      return {
        itemWidth: ir.width,
        nameWidth: nr?.width ?? 0,
      };
    });
    expect(m, 'asset item present').not.toBeNull();
    // Item should be at least the minmax floor (96px) from the auto-fill grid.
    expect(m.itemWidth, 'asset-item width at the auto-fill floor').toBeGreaterThan(80);
    // Name should be wider than the 32px icon — the whole point of the fix.
    expect(m.nameWidth, 'asset-name wider than the 32px icon column').toBeGreaterThan(60);

    const panel = await window.$('.asset-panel');
    await panel.screenshot({ path: path.join(captureDir, 'workspace-after.png') });
  } finally {
    await app.close().catch(() => {});
  }
});

test('outliner search disambiguates entities that share a base name', async () => {
  test.setTimeout(60_000);
  // Reproduce the user-reported scene state: a top-level Bunny placeholder
  // (entity from GISponza.InnoScene) and the GISponza.Bunny.0 mesh entity
  // loaded from the GISponza.Bunny.InnoScene sub-scene. Same pattern for
  // dragon. Without the qualifier rendering, both rows read as 'Bunny' /
  // 'Dragon' duplicates in the search results.
  const entities = [
    { id: 1, name: 'Bunny' },
    { id: 2, name: 'GISponza.Bunny' },
    { id: 3, name: 'GISponza.Bunny.0' },
    { id: 4, name: 'Dragon' },
    { id: 5, name: 'GISponza.Dragon' },
    { id: 6, name: 'GISponza.Dragon.0' },
    { id: 7, name: 'Camera' },
  ];

  const { app, window } = await launchWithMockedScene(entities);
  try {
    // Wait for the entity list to populate from the mocked GET_SCENE reply.
    await window.waitForFunction(
      () => document.querySelectorAll('.entity-item').length >= 7,
      null, { timeout: 10_000 });

    // Type 'bunny' into the outliner search.
    const search = await window.locator('input[placeholder="Search outliner..."]');
    await search.fill('bunny');
    await window.waitForFunction(
      () => document.querySelectorAll('.entity-item').length === 3,
      null, { timeout: 5_000 });

    // Each rendered row must show the qualifier (when present) so the user
    // can tell `Bunny` apart from the `GISponza.*` Bunny entries. Naive's
    // n-text renders as a span — the qualifier carries the .entity-qualifier
    // class, the label is its sibling without that class.
    const rows = await window.evaluate(() => {
      const out = [];
      document.querySelectorAll('.entity-item').forEach(row => {
        const labelHost = row.querySelector('.entity-label');
        if (!labelHost) { out.push({ label: '', qualifier: '' }); return; }
        const children = Array.from(labelHost.children);
        const qualifier = children.find(c => c.classList.contains('entity-qualifier'))?.textContent?.trim() ?? '';
        const label = children.find(c => !c.classList.contains('entity-qualifier'))?.textContent?.trim() ?? '';
        out.push({ label, qualifier });
      });
      return out;
    });

    // Row 1 — top-level placeholder, no qualifier.
    expect(rows[0]).toEqual({ label: 'Bunny', qualifier: '' });
    // Row 2 — sub-scene parent, qualifier 'GISponza', label 'Bunny'.
    expect(rows[1]).toEqual({ label: 'Bunny', qualifier: 'GISponza' });
    // Row 3 — sub-scene mesh instance, qualifier 'GISponza', label 'Bunny.0'.
    expect(rows[2]).toEqual({ label: 'Bunny.0', qualifier: 'GISponza' });

    const outliner = await window.$('.hierarchy-panel');
    await outliner.screenshot({ path: path.join(captureDir, 'outliner-bunny-search.png') });

    // Same pattern for dragon.
    await search.fill('dragon');
    await window.waitForFunction(
      () => document.querySelectorAll('.entity-item').length === 3,
      null, { timeout: 5_000 });
    await outliner.screenshot({ path: path.join(captureDir, 'outliner-dragon-search.png') });

    // Empty search — full list visible, qualifiers still rendered.
    await search.fill('');
    await window.waitForFunction(
      () => document.querySelectorAll('.entity-item').length === 7,
      null, { timeout: 5_000 });
    await outliner.screenshot({ path: path.join(captureDir, 'outliner-no-search.png') });
  } finally {
    await app.close().catch(() => {});
  }
});
