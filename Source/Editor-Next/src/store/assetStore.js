import { reactive } from 'vue'
import { connectionStore } from './connectionStore'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export const assetStore = reactive({
  isImporting: false,
  importProgress: 0,
  currentImportName: '',

  importAsset(filePath) {
    if (!connectionStore.isConnected) return;
    this.isImporting = true;
    this.importProgress = 0;
    this.currentImportName = filePath.split(/[\\\/]/).pop();
    if (ipcRenderer) {
      ipcRenderer.send('engine-message', { type: 'IMPORT_ASSET', path: filePath });
    }
  }
})
