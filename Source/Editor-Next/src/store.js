import { reactive } from 'vue'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export const editorState = reactive({
  entities: [],
  selectedEntity: null,
  selectedEntityId: null,
  isConnected: false,
  isUserInitiatedShutdown: false,
  lastMessage: '',

  // Actions
  reset() {
    console.log('Store: Resetting state...');
    this.entities = [];
    this.selectedEntity = null;
    this.selectedEntityId = null;
  },

  selectEntity(id) {
    if (!this.isConnected) return;
    console.log(`Store: selectEntity called for ID: ${id}`);
    this.selectedEntityId = id;
    if (ipcRenderer) {
      ipcRenderer.send('engine-message', { type: 'GET_ENTITY_DETAILS', id: id });
    }
  },

  updateProperty(data) {
    if (!this.isConnected) return;
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
    if (!this.isConnected) return;
    if (ipcRenderer) {
      ipcRenderer.send('engine-message', { type: 'SAVE_SCENE' });
    }
  },

  restartEngine() {
    this.isUserInitiatedShutdown = true;
    this.reset();
    if (ipcRenderer) {
      ipcRenderer.send('engine-restart');
    }
  },

  stopEngine() {
    this.isUserInitiatedShutdown = true;
    this.reset();
    if (ipcRenderer) {
      ipcRenderer.send('engine-stop');
    }
  }
})
