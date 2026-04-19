import { reactive } from 'vue'
import { connectionStore } from './connectionStore'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

// Mirrors the engine's DevToggleRegistry contents over IPC. The engine
// publishes the list via DEV_TOGGLES (response to LIST_DEV_TOGGLES); the
// editor sends SET_DEV_TOGGLE / TRIGGER_DEV_ACTION to mutate / fire.
export const devToggleStore = reactive({
  toggles: [], // [{ name: string, value: boolean }]
  actions: [], // [{ name: string }]

  refresh() {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'LIST_DEV_TOGGLES' })
  },

  setToggle(name, value) {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'SET_DEV_TOGGLE', name, value })
    // Optimistic local update so the UI reflects the click immediately;
    // the next LIST_DEV_TOGGLES response will reconcile if the engine
    // refused or queued the change.
    const t = this.toggles.find((x) => x.name === name)
    if (t) t.value = value
  },

  triggerAction(name) {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'TRIGGER_DEV_ACTION', name })
  },

  applySnapshot(payload) {
    this.toggles = payload?.toggles ?? []
    this.actions = payload?.actions ?? []
  },

  reset() {
    this.toggles = []
    this.actions = []
  },
})
