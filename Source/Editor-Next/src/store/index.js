import { uiStore } from './uiStore'
import { connectionStore } from './connectionStore'
import { sceneStore } from './sceneStore'
import { assetStore } from './assetStore'

export { uiStore, connectionStore, sceneStore, assetStore }

// Legacy export to maintain backward compatibility during migration
export const editorState = {
  // UI
  get themeFlavor() { return uiStore.themeFlavor },
  set themeFlavor(v) { uiStore.themeFlavor = v },
  setTheme: (flavor) => uiStore.setTheme(flavor),

  // Connection
  get isConnected() { return connectionStore.isConnected },
  set isConnected(v) { connectionStore.isConnected = v },
  get isUserInitiatedShutdown() { return connectionStore.isUserInitiatedShutdown },
  set isUserInitiatedShutdown(v) { connectionStore.isUserInitiatedShutdown = v },
  get lastMessage() { return connectionStore.lastMessage },
  set lastMessage(v) { connectionStore.lastMessage = v },
  restartEngine: () => {
    editorState.reset();
    connectionStore.restartEngine();
  },
  stopEngine: () => {
    editorState.reset();
    connectionStore.stopEngine();
  },

  // Scene
  get entities() { return sceneStore.entities },
  set entities(v) { sceneStore.entities = v },
  get selectedEntity() { return sceneStore.selectedEntity },
  set selectedEntity(v) { sceneStore.selectedEntity = v },
  get selectedEntityId() { return sceneStore.selectedEntityId },
  set selectedEntityId(v) { sceneStore.selectedEntityId = v },
  selectEntity: (id) => sceneStore.selectEntity(id),
  updateProperty: (data) => sceneStore.updateProperty(data),
  saveScene: () => sceneStore.saveScene(),

  // Assets
  get isImporting() { return assetStore.isImporting },
  set isImporting(v) { assetStore.isImporting = v },
  get importProgress() { return assetStore.importProgress },
  set importProgress(v) { assetStore.importProgress = v },
  get currentImportName() { return assetStore.currentImportName },
  set currentImportName(v) { assetStore.currentImportName = v },
  importAsset: (path) => assetStore.importAsset(path),

  // Global Reset (combines domain resets)
  reset() {
    connectionStore.reset();
    sceneStore.reset();
  }
}
