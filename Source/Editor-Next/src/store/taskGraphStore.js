import { reactive } from 'vue'
import { connectionStore } from './connectionStore'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export const taskGraphStore = reactive({
  threads: [], // [{ index, reports: [{ name, startTime, finishTime }] }]

  refresh() {
    if (!connectionStore.isConnected || !ipcRenderer) return
    ipcRenderer.send('engine-message', { type: 'LIST_TASKS' })
  },

  applySnapshot(payload) {
    this.threads = payload?.threads ?? []
  },

  clear() {
    this.threads = []
  },
})
