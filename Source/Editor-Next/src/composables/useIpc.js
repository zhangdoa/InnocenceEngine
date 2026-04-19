/**
 * Editor ↔ engine IPC — request/reply + event bus on top of WebSocket.
 *
 * Wire envelopes (all three fields mandatory):
 *
 *   REQUEST  { envelope: 'request', id: number, type: string, payload: object }
 *   REPLY    { envelope: 'reply',   id: number, status: 'ok'|'err', result?: any, error?: { code, message } }
 *   EVENT    { envelope: 'event',   type: string, payload: object }
 *
 * `id` is a client-generated monotonic integer; the engine echoes the same id
 * on the reply. Requests time out after 30s by default and reject with an
 * IpcError so callers never leak pending promises across disconnects.
 *
 * The composable is a thin hook around a process-wide singleton (one router,
 * one inflight map, one event bus). `useIpc()` inside a component returns
 * `request(type, payload)` and `on(eventType, handler)`; listeners registered
 * through `on()` are auto-torn-down in `onBeforeUnmount`. That keeps HMR
 * cycles from leaking handlers — every remount registers, every unmount
 * removes the specific handler ref, not a nuke-everything
 * removeAllListeners.
 *
 * `engine-connected` is a main-process-emitted Electron channel, not a wire
 * event. It's handled here so stores can react to connect/disconnect via the
 * same event bus: listen with `on('engine-connected', ({ connected }) => …)`.
 */

import { onBeforeUnmount } from 'vue'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export class IpcError extends Error {
  constructor(code, message, requestType) {
    super(message)
    this.name = 'IpcError'
    this.code = code
    this.requestType = requestType
  }
}

// Singletons (one per renderer process).
const inflight = new Map() // id → { resolve, reject, timer, type }
const eventBus = new Map() // type → Set<handler>
let nextRequestId = 1
let routerInstalled = false

function emit(type, payload) {
  const handlers = eventBus.get(type)
  if (!handlers || handlers.size === 0) return
  // Snapshot so a handler unsubscribing during dispatch doesn't mutate the set we're iterating.
  for (const h of [...handlers]) {
    try { h(payload) } catch (e) { console.error(`useIpc event handler (${type}) threw:`, e) }
  }
}

function rejectAllInflight(err) {
  for (const [, entry] of inflight) {
    clearTimeout(entry.timer)
    entry.reject(err)
  }
  inflight.clear()
}

function routeEngineMessage(msg) {
  if (!msg || typeof msg !== 'object') return
  if (msg.envelope === 'reply') {
    const entry = inflight.get(msg.id)
    if (!entry) {
      // Stale reply (request already timed out, or unknown id).
      console.warn('useIpc: reply for unknown id', msg.id, msg)
      return
    }
    clearTimeout(entry.timer)
    inflight.delete(msg.id)
    if (msg.status === 'ok') {
      entry.resolve(msg.result)
    } else {
      const code = msg.error?.code || 'ERR'
      const m = msg.error?.message || 'Engine error'
      entry.reject(new IpcError(code, m, entry.type))
    }
    return
  }
  if (msg.envelope === 'event') {
    emit(msg.type, msg.payload ?? {})
    return
  }
  console.warn('useIpc: message without a known envelope — dropped', msg)
}

function installRouter() {
  if (routerInstalled || !ipcRenderer) return
  routerInstalled = true
  ipcRenderer.on('engine-message', (_event, msg) => routeEngineMessage(msg))
  ipcRenderer.on('engine-connected', (_event, connected) => {
    if (!connected) {
      rejectAllInflight(new IpcError('DISCONNECTED', 'Engine disconnected mid-request'))
    }
    emit('engine-connected', { connected })
  })
  ipcRenderer.on('connection-status', (_event, payload) => {
    emit('connection-status', payload)
  })
  ipcRenderer.on('files-selected', (_event, paths) => {
    emit('files-selected', { paths })
  })
}

installRouter()

/**
 * Send a request. Resolves with the engine's `result`; rejects with an
 * IpcError on status=err, TIMEOUT, DISCONNECTED, or NO_IPC.
 */
export function request(type, payload = {}, { timeoutMs = 30000 } = {}) {
  return new Promise((resolve, reject) => {
    if (!ipcRenderer) {
      reject(new IpcError('NO_IPC', 'ipcRenderer unavailable (running outside Electron?)', type))
      return
    }
    const id = nextRequestId++
    const timer = setTimeout(() => {
      if (!inflight.has(id)) return
      inflight.delete(id)
      reject(new IpcError('TIMEOUT', `Request "${type}" (#${id}) timed out after ${timeoutMs}ms`, type))
    }, timeoutMs)
    inflight.set(id, { resolve, reject, timer, type })
    ipcRenderer.send('engine-message', { envelope: 'request', id, type, payload })
  })
}

/**
 * Subscribe to an event. Returns an unsubscribe function.
 * Prefer `useIpc().on(...)` inside a component — that auto-unsubscribes on
 * unmount. This bare export is for stores or other non-component consumers.
 */
export function on(type, handler) {
  let set = eventBus.get(type)
  if (!set) {
    set = new Set()
    eventBus.set(type, set)
  }
  set.add(handler)
  return () => {
    const s = eventBus.get(type)
    if (!s) return
    s.delete(handler)
    if (s.size === 0) eventBus.delete(type)
  }
}

/**
 * Component-scoped IPC hook. Listeners registered through the returned `on`
 * are removed automatically in `onBeforeUnmount`, surviving HMR cycles.
 */
export function useIpc() {
  const unsubs = []
  const scopedOn = (type, handler) => {
    const unsub = on(type, handler)
    unsubs.push(unsub)
    return unsub
  }
  onBeforeUnmount(() => {
    for (const u of unsubs) u()
    unsubs.length = 0
  })
  return { request, on: scopedOn }
}

// Expose the module-level primitives on window so Playwright contract tests
// can exercise the wire envelope in isolation (tests/ipc-contract.spec.js).
// Vite bundles useIpc into the app chunk, so page.evaluate can't reach it via
// dynamic import from /src/…; a window binding is the cheapest test hook.
// Zero impact on production because no production code path reads __innoIpc.
if (typeof window !== 'undefined') {
  window.__innoIpc = { request, on, IpcError }
}
