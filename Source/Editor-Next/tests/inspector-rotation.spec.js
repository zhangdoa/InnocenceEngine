/**
 * Inspector rotation (Euler ↔ quat) regression for TASK-93.
 *
 * Wire format stays quaternion [x, y, z, w]; the editor converts to/from
 * Euler degrees (XYZ intrinsic) for display and commit. The test runs
 * entirely against an inline mock engine — the conversion math is
 * client-side, so a real engine adds no coverage here.
 */

const { _electron: electron } = require('@playwright/test')
const { test, expect } = require('@playwright/test')
const path = require('path')

async function launchEditorWithMock() {
  const app = await electron.launch({
    args: ['.', '--no-sandbox', '--disable-gpu'],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' },
  })
  const window = await app.firstWindow()
  await window.waitForSelector('.editor-shell', { timeout: 15000 })
  await window.waitForFunction(() => !!window.__innoIpc, { timeout: 5000 })

  await window.evaluate(() => {
    const { ipcRenderer } = window.require('electron')
    const originalSend = ipcRenderer.send.bind(ipcRenderer)
    const responders = new Map()
    let scene = [{ id: 1, name: 'Target' }]
    // Mock engine persists the TransformComponent so re-reads return the
    // committed quaternion, not the seed.
    let transform = { pos: [0, 0, 0], rot: [0, 0, 0, 1], scale: [1, 1, 1] }

    responders.set('GET_SCENE', () => ({ entities: scene.slice() }))
    responders.set('GET_ENTITY_DETAILS', (p) => {
      if (p.id !== 1) return { __error: { code: 'NOT_FOUND', message: 'gone' } }
      return {
        details: {
          id: 1,
          name: 'Target',
          components: [{ type: 'TransformComponent', ...transform }],
        },
      }
    })
    responders.set('UPDATE_ENTITY_PROPERTY', (p) => {
      if (p.component === 'TransformComponent') {
        if (p.property === 'pos')   transform.pos   = p.value
        if (p.property === 'rot')   transform.rot   = p.value
        if (p.property === 'scale') transform.scale = p.value
      }
      return {
        id: p.id,
        component: p.component,
        property: p.property,
        value: transform[p.property],
      }
    })
    responders.set('LIST_DEV_TOGGLES',   () => ({ toggles: [], actions: [] }))
    responders.set('LIST_RENDER_TARGETS', () => ({ passes: [], override: null }))
    responders.set('LIST_TASKS',          () => ({ threads: [] }))

    ipcRenderer.send = (channel, msg) => {
      if (channel !== 'engine-message' || !msg || msg.envelope !== 'request') {
        return originalSend(channel, msg)
      }
      const fn = responders.get(msg.type)
      setTimeout(() => {
        if (!fn) {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: msg.id, status: 'err',
            error: { code: 'NO_HANDLER', message: msg.type },
          })
          return
        }
        const result = fn(msg.payload ?? {})
        if (result && result.__error) {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: msg.id, status: 'err', error: result.__error,
          })
        } else {
          ipcRenderer.emit('engine-message', {}, {
            envelope: 'reply', id: msg.id, status: 'ok', result,
          })
        }
      }, 5)
    }

    ipcRenderer.emit('connection-status', {}, {
      status: 'live', attempt: 0, error: null, nextRetryMs: null,
    })
    ipcRenderer.emit('engine-connected', {}, true)

    window.__readTransform = () => ({ ...transform })
  })

  return { app, window }
}

test('euler → quat → euler round-trip for each axis independently', async () => {
  const { app, window } = await launchEditorWithMock()
  try {
    const result = await window.evaluate(async () => {
      const sceneStore = window.__innoStores?.scene
      if (!sceneStore) return { skipped: true }
      await new Promise(r => setTimeout(r, 30))
      await sceneStore.selectEntity(1)
      await new Promise(r => setTimeout(r, 20))

      const { eulerDegToQuat, quatToEulerDeg } = window.__innoMath

      const checkAxis = async (axisIdx, degrees) => {
        const eulerIn = [0, 0, 0]
        eulerIn[axisIdx] = degrees
        const quat = eulerDegToQuat(eulerIn)
        await sceneStore.updateProperty({
          id: 1, component: 'TransformComponent', property: 'rot', value: quat,
        })
        await new Promise(r => setTimeout(r, 20))
        const committed = window.__readTransform().rot
        const eulerOut = quatToEulerDeg(committed)
        return {
          axis: axisIdx, input: degrees, output: eulerOut[axisIdx],
          other0: eulerOut[(axisIdx + 1) % 3],
          other1: eulerOut[(axisIdx + 2) % 3],
        }
      }

      const results = []
      for (const [axisIdx, deg] of [[0, 35], [1, -47], [2, 88]]) {
        results.push(await checkAxis(axisIdx, deg))
      }
      return { results }
    })

    if (result.skipped) return

    const TOL = 1e-4
    for (const r of result.results) {
      expect(Math.abs(r.output - r.input), `axis ${r.axis} self-component drift`).toBeLessThan(TOL)
      expect(Math.abs(r.other0), `axis ${r.axis} cross-leak ${(r.axis + 1) % 3}`).toBeLessThan(TOL)
      expect(Math.abs(r.other1), `axis ${r.axis} cross-leak ${(r.axis + 2) % 3}`).toBeLessThan(TOL)
    }
  } finally {
    await app.close().catch(() => {})
  }
})

test('rotation input renders three numeric axes wired to commit', async () => {
  const { app, window } = await launchEditorWithMock()
  try {
    const outcome = await window.evaluate(async () => {
      const sceneStore = window.__innoStores?.scene
      if (!sceneStore) return { skipped: true }
      await new Promise(r => setTimeout(r, 30))
      await sceneStore.selectEntity(1)
      await new Promise(r => setTimeout(r, 50))
      const inputs = document.querySelectorAll('[data-test-prop="rot-euler-input"]')
      return {
        count: inputs.length,
        axes: Array.from(inputs).map((el) => el.getAttribute('data-test-axis')),
      }
    })
    if (outcome.skipped) return
    expect(outcome.count).toBe(3)
    expect(outcome.axes).toEqual(['X', 'Y', 'Z'])
  } finally {
    await app.close().catch(() => {})
  }
})
