import { reactive } from 'vue'

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

export const connectionStore = reactive({
  isConnected: false,
  isUserInitiatedShutdown: false,
  lastMessage: '',

  reset() {
    console.log('ConnectionStore: Resetting connection state...');
    this.isConnected = false;
    this.lastMessage = '';
  },

  stopEngine() {
    this.isUserInitiatedShutdown = true;
    if (ipcRenderer) {
      ipcRenderer.send('engine-stop');
    }
  },

  restartEngine() {
    this.isUserInitiatedShutdown = true;
    if (ipcRenderer) {
      ipcRenderer.send('engine-restart');
    }
  }
})
