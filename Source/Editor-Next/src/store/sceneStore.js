import { reactive } from 'vue'
import { connectionStore } from './connectionStore'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export const sceneStore = reactive({
  entities: [],
  selectedEntity: null,
  selectedEntityId: null,

  reset() {
    console.log('SceneStore: Resetting scene state...');
    this.entities = [];
    this.selectedEntity = null;
    this.selectedEntityId = null;
  },

  selectEntity(id) {
    if (!connectionStore.isConnected) return;
    console.log(`SceneStore: selectEntity called for ID: ${id}`);
    this.selectedEntityId = id;
    if (ipcRenderer) {
      ipcRenderer.send('engine-message', { type: 'GET_ENTITY_DETAILS', id: id });
    }
  },

  updateProperty(data) {
    if (!connectionStore.isConnected) return;
    if (ipcRenderer) {
      // Clone data to remove Vue proxies before sending over IPC
      const plainData = JSON.parse(JSON.stringify(data));
      ipcRenderer.send('engine-message', {
        type: 'UPDATE_ENTITY_PROPERTY',
        ...plainData
      });
    }
  },

  saveScene() {
    if (!connectionStore.isConnected) return;
    if (ipcRenderer) {
      ipcRenderer.send('engine-message', { type: 'SAVE_SCENE' });
    }
  }
})
