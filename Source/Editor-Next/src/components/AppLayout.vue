<template>
  <div class="editor-shell">
    <header class="editor-header">
      <div class="header-left">
        <n-menu mode="horizontal" :options="menuOptions" class="menu-bar" />
      </div>
      <div class="header-right">
        <n-space align="center" :size="20">
          <n-button-group size="small">
            <n-button @click="saveScene" secondary title="Save current scene">
              <template #icon><n-icon><component :is="icons.save" /></n-icon></template>
              Save
            </n-button>
            <n-button @click="restartEngine" secondary title="Restart Engine sidecar">
              <template #icon><n-icon><component :is="icons.refresh" /></n-icon></template>
              Restart
            </n-button>
            <n-button @click="stopEngine" type="error" ghost title="Stop Engine sidecar">
              <template #icon><n-icon><component :is="icons.power" /></n-icon></template>
              Stop
            </n-button>
          </n-button-group>
          <n-tag :type="editorState.isConnected ? 'success' : 'error'" size="small" round ghost>
            <template #icon>
              <n-icon>
                <component :is="editorState.isConnected ? icons.checkmark : icons.close" />
              </n-icon>
            </template>
            {{ editorState.isConnected ? 'Live' : 'Offline' }}
          </n-tag>
        </n-space>
      </div>
    </header>

    <main class="dock-container">
      <dockview-vue
        class="dockview-theme-abyssal"
        style="width: 100%; height: 100%;"
        @ready="onDockviewReady"
      >
      </dockview-vue>
    </main>

    <footer class="editor-footer">
      <n-space justify="space-between" align="center" style="width: 100%; height: 100%; padding: 0 12px;">
        <n-text depth="3" style="font-size: 10px; text-transform: uppercase; letter-spacing: 1px;">Ready</n-text>
        <n-text depth="3" class="flex-grow" style="font-size: 11px; font-family: monospace;">
          {{ editorState.lastMessage || 'System Idle' }}
        </n-text>
        <n-text depth="3" style="font-size: 10px;">v0.0.9</n-text>
      </n-space>
    </footer>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, shallowRef, markRaw } from 'vue'
import { 
  NMenu, NButton, NButtonGroup, NTag, NSpace, NText, useMessage, NIcon
} from 'naive-ui'
import { 
  Power, Refresh, SaveOutline, TerminalOutline,
  CheckmarkCircle, CloseCircle
} from '@vicons/ionicons5'
import { DockviewVue } from 'dockview-vue'
import { editorState } from '../store'

import 'dockview-vue/dist/styles/dockview.css'

// Menu Options
const menuOptions = [
  { label: 'Project', key: 'file' },
  { label: 'Scene', key: 'edit' },
  { label: 'Tools', key: 'view' },
  { label: 'Engine', key: 'engine' }
]

// Icon mapping for template
const icons = {
  power: Power,
  refresh: Refresh,
  save: SaveOutline,
  terminal: TerminalOutline,
  checkmark: CheckmarkCircle,
  close: CloseCircle
}

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }
const message = useMessage()

// Track if we have ever connected to avoid first-launch error popup
const hasEverConnected = ref(false)

// Dockview
const dockviewApi = shallowRef()

const setupIpc = () => {
  if (!ipcRenderer) return;

  ipcRenderer.on('engine-connected', (event, connected) => {
    const wasConnected = editorState.isConnected;
    editorState.isConnected = connected
    
    if (connected) {
      hasEverConnected.value = true;
      editorState.isUserInitiatedShutdown = false
      editorState.lastMessage = 'Engine Handshake Successful'
      ipcRenderer.send('engine-message', { type: 'GET_SCENE' })
      message.success('System Online')
    } else {
      editorState.lastMessage = 'Engine Connection Lost'
      editorState.reset()
      
      // Only show error if we were previously connected and it wasn't a planned stop
      if (wasConnected && !editorState.isUserInitiatedShutdown) {
        message.error('Engine Connection Interrupted')
      }
    }
  })

  ipcRenderer.on('engine-message', (event, msg) => {
    console.log(`AppLayout: Received engine message: ${msg.type}`);
    editorState.lastMessage = `Last Msg: ${msg.type}`
    if (msg.type === 'SCENE_DATA') {
      editorState.entities = msg.entities
    } else if (msg.type === 'ENTITY_DETAILS') {
      editorState.selectedEntity = msg.details
    }
  })
}

const onDockviewReady = (event) => {
  dockviewApi.value = event.api
  console.log('Dockview ready, creating panels...');

  // Create detached layout
  const hierarchyPane = event.api.addPanel({
    id: 'hierarchy_panel',
    component: 'hierarchy',
    title: 'Outliner'
  })

  const propertiesPane = event.api.addPanel({
    id: 'properties_panel',
    component: 'properties',
    title: 'Inspector',
    position: { direction: 'right', referencePanel: hierarchyPane, width: 450 }
  })

  const assetsPane = event.api.addPanel({
    id: 'assets_panel',
    component: 'assets',
    title: 'Workspace',
    position: { direction: 'below', referencePanel: hierarchyPane, height: 350 }
  })
  
  console.log('All panels created.');
}

const saveScene = () => {
  editorState.saveScene()
  message.success('State persistent')
}

const restartEngine = () => {
  editorState.restartEngine()
  message.info('Rebooting Engine...')
}

const stopEngine = () => {
  editorState.stopEngine()
  message.warning('Engine Terminated')
}

onMounted(() => {
  setupIpc()
  window.addEventListener('load-scene', (e) => {
    ipcRenderer.send('engine-message', { type: 'LOAD_SCENE', path: e.detail })
  })
})

onUnmounted(() => {
  if (ipcRenderer) {
    ipcRenderer.removeAllListeners('engine-connected')
    ipcRenderer.removeAllListeners('engine-message')
  }
})
</script>

<style scoped>
.editor-shell {
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  background: #24273a; /* Catppuccin Macchiato Base */
  color: #cad3f5; /* Catppuccin Macchiato Text */
}

.editor-header {
  height: 52px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20px;
  background: #1e2030; /* Catppuccin Macchiato Mantle */
  border-bottom: 1px solid #494d64; /* Catppuccin Macchiato Surface1 */
  user-select: none;
}

.header-left { 
  display: flex; 
  align-items: center; 
  gap: 32px;
  flex: 1;
}

.menu-bar { 
  flex: 1;
  min-width: 300px;
  border: none !important; 
  background: transparent !important;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 20px;
  flex-shrink: 0;
  min-width: 400px;
  justify-content: flex-end;
}

.dock-container {
  flex: 1;
  position: relative;
  overflow: hidden;
  background: #24273a;
}

.editor-footer {
  height: 28px;
  flex-shrink: 0;
  background: #1e2030;
  border-top: 1px solid #494d64;
}

.flex-grow { flex: 1; text-align: center; }

/* Naive UI Theme Fixes */
:deep(.n-menu-item-content-header) {
  font-weight: 500;
  font-size: 13px;
}
</style>
