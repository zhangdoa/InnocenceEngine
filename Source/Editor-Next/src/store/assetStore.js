import { reactive } from 'vue'
import { request, on } from '../composables/useIpc'
import { connectionStore } from './connectionStore'

/**
 * Asset import state. Progress / completion events are not wired on the
 * engine side yet — they come in through IMPORT_PROGRESS / IMPORT_FINISHED
 * bus events once emitted. For now the modal closes as soon as the request
 * reply returns (which means "engine accepted the import task"), and clears
 * on disconnect. Real progress stream is phase 6 work.
 */
export const assetStore = reactive({
  isImporting: false,
  importProgress: 0,
  currentImportName: '',

  reset() {
    this.isImporting = false
    this.importProgress = 0
    this.currentImportName = ''
  },

  async importAsset(filePath) {
    if (!connectionStore.isConnected) return
    this.isImporting = true
    this.importProgress = 0
    this.currentImportName = filePath.split(/[\\\/]/).pop()
    try {
      await request('IMPORT_ASSET', { path: filePath })
    } finally {
      // Until the engine emits a real IMPORT_FINISHED event, the modal
      // dismisses on reply. See TASK-90 for the streaming progress path.
      this.isImporting = false
    }
  },
})

on('engine-connected', ({ connected }) => {
  if (!connected) assetStore.reset()
})

on('files-selected', ({ paths }) => {
  if (!Array.isArray(paths)) return
  for (const p of paths) {
    assetStore.importAsset(p).catch(e => console.error('assetStore.importAsset failed:', e))
  }
})

on('IMPORT_PROGRESS', (payload) => {
  if (!payload) return
  assetStore.isImporting = true
  assetStore.importProgress = payload.progress ?? 0
  assetStore.currentImportName = payload.name ?? assetStore.currentImportName
})

on('IMPORT_FINISHED', (payload) => {
  assetStore.reset()
  if (payload) {
    window.dispatchEvent(new CustomEvent('refresh-assets'))
  }
})
