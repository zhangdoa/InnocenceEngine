/**
 * GET ↔ UPDATE symmetry regression for entity properties (TASK-101).
 *
 * Drives the live engine (`--engine=Main`) so the assertions speak for the
 * real EditorService.cpp handlers, not a mock — the whole point of TASK-101
 * is to catch handler-side drift between read and write paths, which a
 * mock can't see by definition.
 *
 * Loads GISponza (10 transforms, 4 lights), iterates every entity returned
 * by GET_SCENE, calls GET_ENTITY_DETAILS, and round-trips every
 * `component.<prop>` through UPDATE_ENTITY_PROPERTY. For each field:
 *
 *   - status=ok and the readback equals the input  → symmetric, pass
 *   - status=err with code=READ_ONLY               → intentional read-only, pass
 *   - status=err with code=BAD_PROPERTY            → asymmetry, FAIL
 *   - status=err with any other code               → contract violation, FAIL
 *
 * READ_ONLY is the discriminated reply that distinguishes "no writer by
 * design" from "writer accidentally missing". Pre-TASK-101, the LightComponent
 * `shape` and `lightType` fields fell through to BAD_PROPERTY — the same
 * code an unknown-property typo emits — so the inspector had no signal that
 * the field was deliberately uneditable vs accidentally stuck.
 *
 * The spec passes the value the engine just returned as the write payload,
 * so a successful round-trip is observably idempotent: any clamp / coerce
 * / drop the engine performs lands in the readback and is asserted against
 * the GET-side value.
 */

const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

const VEC_EQ_TOL = 1e-5;

function valuesMatch(a, b) {
  if (typeof a === 'number' && typeof b === 'number') {
    return Math.abs(a - b) < VEC_EQ_TOL;
  }
  if (Array.isArray(a) && Array.isArray(b)) {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
      if (!valuesMatch(a[i], b[i])) return false;
    }
    return true;
  }
  return a === b;
}

test('GET/UPDATE symmetry across every exposed component property', async () => {
  test.setTimeout(240000);
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
    await window.waitForFunction(() => !!window.__innoIpc, { timeout: 5000 });

    // The engine boot does not auto-load a scene in -mode 2 — push GISponza
    // explicitly. SCENE_UPDATED arrives async after the load completes; wait
    // on it before enumerating entities or GET_SCENE may return empty.
    await window.evaluate(async () => {
      const { request, on } = window.__innoIpc;
      const sceneUpdated = new Promise((resolve) => {
        const off = on('SCENE_UPDATED', () => { off(); resolve(); });
      });
      // Engine resolves the path against its data directory; AssetPanel
      // sends `<currentDir>/<file>`. The Scenes/ subdirectory is required —
      // bare filenames fail to open (TASK-101 first-pass attempt verified
      // this with `Can't open JSON file: GISponza.InnoScene!`).
      await request('LOAD_SCENE', { path: 'ExampleProject/Scenes/GISponza.InnoScene' }, { timeoutMs: 60000 });
      await sceneUpdated;
    });

    const audit = await window.evaluate(async () => {
      const { request, IpcError } = window.__innoIpc;
      const sceneRes = await request('GET_SCENE', {});
      const entities = sceneRes.entities || [];

      // Tagged-union result so the assertion phase can categorise without
      // re-parsing IpcError shape: {kind: 'ok' | 'read-only' | 'fail',
      // expectedReadback?, actualReadback?, errorCode?}.
      const results = [];

      for (const ent of entities) {
        let details;
        try {
          const r = await request('GET_ENTITY_DETAILS', { id: ent.id });
          details = r.details;
        } catch (e) {
          results.push({
            entityId: ent.id, entityName: ent.name,
            component: '<details>', property: '<details>',
            kind: 'fail',
            errorCode: e.code, errorMessage: e.message,
            note: 'GET_ENTITY_DETAILS rejected',
          });
          continue;
        }

        for (const comp of details.components || []) {
          const compType = comp.type;
          for (const propName of Object.keys(comp)) {
            if (propName === 'type') continue;

            const inputValue = comp[propName];
            const payload = {
              id: ent.id,
              component: compType,
              property: propName,
              value: inputValue,
            };
            try {
              const writeRes = await request('UPDATE_ENTITY_PROPERTY', payload);
              // Re-read so the assertion phase sees server-truth, not the
              // write reply (which already does the read-back internally,
              // but we want both layers covered: write reply *and* a fresh
              // GET match.)
              const reread = await request('GET_ENTITY_DETAILS', { id: ent.id });
              const rereadComp = (reread.details.components || [])
                .find(c => c.type === compType);
              const rereadValue = rereadComp ? rereadComp[propName] : null;
              results.push({
                entityId: ent.id, entityName: ent.name,
                component: compType, property: propName,
                kind: 'ok',
                inputValue,
                writeReplyValue: writeRes.value,
                rereadValue,
              });
            } catch (e) {
              const code = e?.code || 'UNKNOWN';
              results.push({
                entityId: ent.id, entityName: ent.name,
                component: compType, property: propName,
                kind: code === 'READ_ONLY' ? 'read-only' : 'fail',
                errorCode: code,
                errorMessage: e?.message || String(e),
                inputValue,
              });
            }
          }
        }
      }

      return { entityCount: entities.length, results };
    });

    // ── Assertions ───────────────────────────────────────────────────────
    // Test passes when every (entity, component, property) tuple is either
    // 'ok' (with matching readback) or 'read-only' with code=READ_ONLY.
    // Any 'fail' is reported in full so the failure message is useful.

    const failures = audit.results.filter(r => r.kind === 'fail');
    const readOnly = audit.results.filter(r => r.kind === 'read-only');
    const oks      = audit.results.filter(r => r.kind === 'ok');

    // Surface a compact per-property summary in the test output regardless
    // of pass/fail — useful when debugging or auditing wire shape changes.
    console.log('=== entity-property-symmetry summary ===');
    console.log(`Entities scanned: ${audit.entityCount}`);
    console.log(`Round-trips OK:   ${oks.length}`);
    console.log(`READ_ONLY:        ${readOnly.length}`);
    console.log(`Failures:         ${failures.length}`);
    const seenKey = new Set();
    for (const r of audit.results) {
      const key = `${r.component}.${r.property}|${r.kind}`;
      if (seenKey.has(key)) continue;
      seenKey.add(key);
      const tag = r.kind === 'fail' ? `FAIL(${r.errorCode})` : r.kind.toUpperCase();
      console.log(`  ${tag.padEnd(14)} ${r.component}.${r.property}`);
    }

    // Hard assertion: zero unexpected failures.
    expect(
      failures,
      'every (component, property) returned by GET_ENTITY_DETAILS must round-trip ' +
      'through UPDATE_ENTITY_PROPERTY successfully or return code=READ_ONLY; the ' +
      'following tuples violated that contract:\n' +
      JSON.stringify(failures, null, 2),
    ).toEqual([]);

    // Drift check: for every successful round-trip, the value the engine
    // wrote and re-read must equal the value GET_ENTITY_DETAILS just gave us.
    // Mismatches indicate clamping, dropped fields, or write-then-different-
    // read drift in the handler.
    const drift = oks.filter(r =>
      !valuesMatch(r.inputValue, r.writeReplyValue) ||
      !valuesMatch(r.inputValue, r.rereadValue)
    );
    expect(
      drift,
      'UPDATE_ENTITY_PROPERTY readback (or subsequent GET_ENTITY_DETAILS) drifted ' +
      'from the value just submitted; the following tuples drifted:\n' +
      JSON.stringify(drift, null, 2),
    ).toEqual([]);

    // Sanity: GISponza is non-trivial — bail loudly if we somehow audited
    // zero entities (would otherwise vacuous-pass).
    expect(audit.entityCount).toBeGreaterThan(0);
    expect(oks.length + readOnly.length).toBeGreaterThan(0);
  } finally {
    await electronApp.close().catch(() => {});
  }
});
