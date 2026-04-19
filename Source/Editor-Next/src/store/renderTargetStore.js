import { reactive } from 'vue'
import { request, on } from '../composables/useIpc'
import { connectionStore } from './connectionStore'

/**
 * Mirrors the engine's render-pass / RT enumeration. LIST_RENDER_TARGETS
 * fetches the full list (plus the current override if any). SET_VIEWPORT_SOURCE
 * with {pass, rtIndex} activates an override; no payload resets it.
 */
export const renderTargetStore = reactive({
  passes: [],     // [{ name, targets: [{ index, name }] }]
  override: null, // { pass, rtIndex } | null

  reset() {
    this.passes = []
    this.override = null
  },

  async refresh() {
    if (!connectionStore.isConnected) return
    const result = await request('LIST_RENDER_TARGETS')
    this.passes = result?.passes ?? []
    this.override = result?.override ?? null
  },

  async setOverride(passName, rtIndex) {
    if (!connectionStore.isConnected) return
    const result = await request('SET_VIEWPORT_SOURCE', { pass: passName, rtIndex })
    this.override = result && result.pass !== undefined
      ? { pass: result.pass, rtIndex: result.rtIndex }
      : null
  },

  async clearOverride() {
    if (!connectionStore.isConnected) return
    await request('SET_VIEWPORT_SOURCE')
    this.override = null
  },

  onConnect() {
    this.refresh().catch(e => console.error('renderTargetStore.refresh on connect:', e))
  },
  onDisconnect() {
    this.reset()
  },
})

on('engine-connected', ({ connected }) => {
  if (connected) renderTargetStore.onConnect()
  else renderTargetStore.onDisconnect()
})
