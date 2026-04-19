import { reactive } from 'vue'
import { on } from '../composables/useIpc'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

/**
 * Connection state owned here; set by the main process via the
 * 'engine-connected' Electron channel, which useIpc translates into an
 * event-bus emission (same bus as engine wire events, distinct event type).
 *
 * `stopEngine` / `restartEngine` go to main.js on dedicated channels — those
 * are Electron lifecycle controls, not engine wire messages.
 */
export const connectionStore = reactive({
  isConnected: false,
  isUserInitiatedShutdown: false,
  lastMessage: '',

  reset() {
    this.isConnected = false
    this.lastMessage = ''
  },

  stopEngine() {
    this.isUserInitiatedShutdown = true
    if (ipcRenderer) ipcRenderer.send('engine-stop')
  },

  restartEngine() {
    this.isUserInitiatedShutdown = true
    if (ipcRenderer) ipcRenderer.send('engine-restart')
  },
})

// Singleton: subscribe once at module load. Not tied to a component lifecycle.
on('engine-connected', ({ connected }) => {
  connectionStore.isConnected = connected
  if (connected) {
    connectionStore.isUserInitiatedShutdown = false
    connectionStore.lastMessage = 'Engine handshake successful'
  } else {
    connectionStore.lastMessage = 'Engine connection lost'
    connectionStore.reset()
  }
})
