import { reactive, computed } from 'vue'
import { on } from '../composables/useIpc'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

/**
 * Connection-lifecycle state machine (renderer side). Mirrors main.js.
 *
 *   idle → connecting → live → lost → connecting … → giving-up
 *                                 ↓
 *   stopping ← (user action from any state)
 *
 * `status` is the single source of truth. `isConnected` is a derived
 * computed that treats `live` as the only "connected" state; domain stores
 * watch it (via Vue `watch`) to refresh/reset on transitions.
 *
 * main.js pushes `connection-status` events with the shape:
 *   { status, attempt, error, nextRetryMs }
 * and a legacy `engine-connected` binary signal for subscribers that haven't
 * migrated to status yet.
 */

export const VALID_STATUSES = Object.freeze([
  'idle',
  'connecting',
  'live',
  'lost',
  'stopping',
  'giving-up',
])

export const connectionStore = reactive({
  status: 'idle',
  attempt: 0,
  error: null,
  nextRetryMs: null,
  lastMessage: '',
  isUserInitiatedShutdown: false,

  reset() {
    this.status = 'idle'
    this.attempt = 0
    this.error = null
    this.nextRetryMs = null
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

  /** Called from the UI when the status machine is in `giving-up` state. */
  retryNow() {
    this.isUserInitiatedShutdown = false
    if (ipcRenderer) ipcRenderer.send('engine-retry')
  },
})

// `isConnected` exposed as an instance getter so panels that still read it
// as a reactive boolean (`connectionStore.isConnected`) continue to work —
// the getter re-runs on any read and Vue tracks the underlying `status`.
Object.defineProperty(connectionStore, 'isConnected', {
  enumerable: true,
  get() { return this.status === 'live' },
})

// Subscribe once at module load. Renderer-local event bus delivered by
// useIpc's router — `connection-status` payloads come from main.js.
on('connection-status', (payload) => {
  if (!payload || typeof payload !== 'object') return
  if (!VALID_STATUSES.includes(payload.status)) return
  connectionStore.status = payload.status
  connectionStore.attempt = payload.attempt ?? 0
  connectionStore.error = payload.error ?? null
  connectionStore.nextRetryMs = payload.nextRetryMs ?? null
  connectionStore.lastMessage = describe(payload)
  if (payload.status === 'live' || payload.status === 'idle' || payload.status === 'stopping') {
    connectionStore.isUserInitiatedShutdown = payload.status !== 'live'
  }
})

function describe(payload) {
  switch (payload.status) {
    case 'idle':       return 'Engine idle'
    case 'connecting': return payload.attempt > 0
      ? `Reconnecting (attempt ${payload.attempt})…`
      : 'Connecting to engine…'
    case 'live':       return 'Engine handshake successful'
    case 'lost':       return payload.nextRetryMs
      ? `Connection lost — retrying in ${Math.round(payload.nextRetryMs / 1000)}s`
      : 'Connection lost'
    case 'stopping':   return 'Stopping engine…'
    case 'giving-up':  return payload.error
      ? `Engine unreachable: ${payload.error}`
      : 'Engine unreachable — give up'
    default:           return ''
  }
}

export const connectionDisplay = computed(() => connectionStore.lastMessage)
