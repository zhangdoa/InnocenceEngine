import { reactive } from 'vue'
import { request, on } from '../composables/useIpc'
import { connectionStore } from './connectionStore'

/**
 * Mirrors the engine's DevToggleRegistry. LIST_DEV_TOGGLES fetches the full
 * snapshot; SET_DEV_TOGGLE / TRIGGER_DEV_ACTION mutate. Optimistic local
 * update keeps the UI responsive; the next refresh reconciles if the engine
 * refused or coerced the value.
 */
export const devToggleStore = reactive({
  toggles: [], // [{ name: string, value: boolean }]
  actions: [], // [{ name: string }]

  reset() {
    this.toggles = []
    this.actions = []
  },

  async refresh() {
    if (!connectionStore.isConnected) return
    const result = await request('LIST_DEV_TOGGLES')
    this.toggles = result?.toggles ?? []
    this.actions = result?.actions ?? []
  },

  async setToggle(name, value) {
    if (!connectionStore.isConnected) return
    const t = this.toggles.find((x) => x.name === name)
    if (t) t.value = value // optimistic
    const committed = await request('SET_DEV_TOGGLE', { name, value })
    if (committed && t) t.value = committed.value
  },

  async triggerAction(name) {
    if (!connectionStore.isConnected) return
    await request('TRIGGER_DEV_ACTION', { name })
  },

  onConnect() {
    this.refresh().catch(e => console.error('devToggleStore.refresh on connect:', e))
  },
  onDisconnect() {
    this.reset()
  },
})

on('engine-connected', ({ connected }) => {
  if (connected) devToggleStore.onConnect()
  else devToggleStore.onDisconnect()
})
