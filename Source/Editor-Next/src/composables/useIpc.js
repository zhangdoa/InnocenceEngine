import { onMounted, onUnmounted } from 'vue'
import { useMessage } from 'naive-ui'
import { connectionStore } from '../store/connectionStore'
import { sceneStore } from '../store/sceneStore'
import { assetStore } from '../store/assetStore'
import { devToggleStore } from '../store/devToggleStore'

export function useIpc() {
  const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }
  const message = useMessage()

  const setupIpc = () => {
    if (!ipcRenderer) return

    // Connection events
    ipcRenderer.on('engine-connected', (event, connected) => {
      connectionStore.isConnected = connected
      if (connected) {
        connectionStore.isUserInitiatedShutdown = false
        connectionStore.lastMessage = 'Engine Handshake Successful'
        ipcRenderer.send('engine-message', { type: 'GET_SCENE' })
        ipcRenderer.send('engine-message', { type: 'LIST_DEV_TOGGLES' })
        message.success('System Online')
      } else {
        connectionStore.lastMessage = 'Engine Connection Lost'
        connectionStore.reset()
      }
    })

    // Engine message routing
    ipcRenderer.on('engine-message', (event, msg) => {
      switch (msg.type) {
        case 'SCENE_DATA':
          sceneStore.entities = msg.entities
          break
        case 'ENTITY_DETAILS':
          sceneStore.selectedEntity = msg.details
          break
        case 'IMPORT_PROGRESS':
          assetStore.isImporting = true
          assetStore.importProgress = msg.progress
          assetStore.currentImportName = msg.name
          break
        case 'IMPORT_FINISHED':
          assetStore.isImporting = false
          assetStore.importProgress = 0
          if (msg.success) {
            message.success(`Import complete: ${msg.name}`)
            window.dispatchEvent(new CustomEvent('refresh-assets'))
          } else {
            message.error(`Import failed: ${msg.name}`)
          }
          break
        case 'DEV_TOGGLES':
          devToggleStore.applySnapshot(msg)
          break
        case 'HELLO_REPLY':
          // Handled by main process for handle duplication,
          // but we can log it here if needed
          break
      }
    })

    // File selection
    ipcRenderer.on('files-selected', (event, filePaths) => {
      filePaths.forEach(path => assetStore.importAsset(path))
    })
  }

  const onLoadScene = (e) => {
    if (!ipcRenderer) return
    if (!connectionStore.isConnected) {
      message.warning('Engine offline; scene load ignored')
      return
    }
    ipcRenderer.send('engine-message', { type: 'LOAD_SCENE', path: e.detail })
    message.info(`Loading ${e.detail}…`)
  }

  const cleanupIpc = () => {
    if (ipcRenderer) {
      ipcRenderer.removeAllListeners('engine-connected')
      ipcRenderer.removeAllListeners('engine-message')
      ipcRenderer.removeAllListeners('files-selected')
    }
    window.removeEventListener('load-scene', onLoadScene)
  }

  onMounted(() => {
    setupIpc()
    window.addEventListener('load-scene', onLoadScene)
  })

  onUnmounted(() => {
    cleanupIpc()
  })

  return {
    ipcRenderer
  }
}
