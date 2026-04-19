import { reactive } from 'vue'
import { request, on, IpcError } from '../composables/useIpc'
import { connectionStore } from './connectionStore'

/**
 * Scene state. UI reads; store mutates itself only from engine replies.
 * Every mutation is an async action returning the reply promise so callers
 * can await completion (and toasts/error boundaries can surface failures).
 */
export const sceneStore = reactive({
  entities: [],
  selectedEntity: null,
  selectedEntityId: null,
  isLoading: false,     // true between LOAD_SCENE dispatch and SCENE_UPDATED
  currentScene: '',     // populated from the engine's SCENE_UPDATED payload

  reset() {
    this.entities = []
    this.selectedEntity = null
    this.selectedEntityId = null
    this.isLoading = false
    this.currentScene = ''
  },

  async refresh() {
    if (!connectionStore.isConnected) return
    const { entities } = await request('GET_SCENE')
    this.entities = entities ?? []
  },

  async selectEntity(id) {
    if (!connectionStore.isConnected) return
    this.selectedEntityId = id
    try {
      const { details } = await request('GET_ENTITY_DETAILS', { id })
      // A different selection may have raced us — only commit if the reply
      // still matches the user's current intent.
      if (this.selectedEntityId === id) this.selectedEntity = details
    } catch (e) {
      if (e instanceof IpcError && e.code === 'NOT_FOUND') {
        if (this.selectedEntityId === id) {
          this.selectedEntity = null
          this.selectedEntityId = null
        }
      } else {
        throw e
      }
    }
  },

  async updateProperty(data) {
    if (!connectionStore.isConnected) return
    // Clone to strip Vue reactivity proxies before crossing the IPC boundary.
    const plainData = JSON.parse(JSON.stringify(data))
    const committed = await request('UPDATE_ENTITY_PROPERTY', plainData)
    // Merge the post-commit value into selectedEntity if this property
    // belongs to the currently viewed entity — the engine may clamp or
    // normalize what the user typed.
    if (
      this.selectedEntity &&
      committed &&
      this.selectedEntity.id === committed.id &&
      Array.isArray(this.selectedEntity.components)
    ) {
      const comp = this.selectedEntity.components.find(c => c.type === committed.component)
      if (comp) comp[committed.property] = committed.value
    }
  },

  async saveScene() {
    if (!connectionStore.isConnected) return
    await request('SAVE_SCENE')
  },

  async createEntity(name = 'Entity') {
    if (!connectionStore.isConnected) return
    const { entities } = await request('ENTITY_CREATE', { name })
    this.entities = entities ?? []
  },

  async deleteEntity(id) {
    if (!connectionStore.isConnected) return
    const { entities } = await request('ENTITY_DELETE', { id })
    this.entities = entities ?? []
    if (this.selectedEntityId === id) {
      this.selectedEntity = null
      this.selectedEntityId = null
    }
  },

  async renameEntity(id, name) {
    if (!connectionStore.isConnected) return
    const { entities } = await request('ENTITY_RENAME', { id, name })
    this.entities = entities ?? []
  },

  /**
   * Fire a LOAD_SCENE request. The engine replies immediately with the
   * accepted path — the actual load is async; the SCENE_UPDATED event
   * (broadcast from SceneService's loaded-callback) is the real completion
   * signal. This action flips `isLoading` for the duration so UI can show
   * a pending state.
   */
  async loadScene(path) {
    if (!connectionStore.isConnected) return
    this.isLoading = true
    try {
      await request('LOAD_SCENE', { path })
      // Safety net: if SCENE_UPDATED never arrives (engine load failed
      // silently before reaching our callback), clear `isLoading` after
      // a grace window so the UI doesn't stay pending forever. A real
      // engine-side failure here is worth its own backlog task.
      setTimeout(() => { this.isLoading = false }, 15000)
    } catch (e) {
      this.isLoading = false
      throw e
    }
  },

  onConnect() {
    this.refresh().catch(e => console.error('sceneStore.refresh on connect:', e))
  },
  onDisconnect() {
    this.reset()
  },
})

on('engine-connected', ({ connected }) => {
  if (connected) sceneStore.onConnect()
  else sceneStore.onDisconnect()
})

on('SCENE_UPDATED', (payload) => {
  if (payload?.scene) sceneStore.currentScene = payload.scene
  sceneStore.isLoading = false
  sceneStore.refresh().catch(e => console.error('sceneStore.refresh on SCENE_UPDATED:', e))
})
