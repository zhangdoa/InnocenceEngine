import { reactive } from 'vue'
import { connectionStore } from './connectionStore'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

// Mirrors the engine's render-pass / RT enumeration. The engine publishes
// the list via RENDER_TARGETS (response to LIST_RENDER_TARGETS); the
// editor sends SET_VIEWPORT_SOURCE (or SET_VIEWPORT_SOURCE with no args
// to reset) to drive ViewportSourceOverride.
export const renderTargetStore = reactive({
  passes: [],     // [{ name, targets: [{ index, name }] }]
  override: null, // { pass, rtIndex } | null

  refresh() {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'LIST_RENDER_TARGETS' })
  },

  setOverride(passName, rtIndex) {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'SET_VIEWPORT_SOURCE', pass: passName, rtIndex })
    this.override = { pass: passName, rtIndex }
  },

  reset() {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'SET_VIEWPORT_SOURCE' })
    this.override = null
  },

  applySnapshot(payload) {
    this.passes = payload?.passes ?? []
    this.override = payload?.override ?? null
  },

  clear() {
    this.passes = []
    this.override = null
  },
})
