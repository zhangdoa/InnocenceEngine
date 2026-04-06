<template>
  <n-layout class="editor-shell" position="absolute">
    <n-layout-header bordered class="editor-header">
      <div class="header-left">
        <div class="logo">InnocenceEngine</div>
        <n-menu mode="horizontal" :options="menuOptions" class="menu-bar" />
      </div>
      <div class="header-right">
        <n-space align="center">
          <n-button-group size="small">
            <n-button @click="saveScene" secondary title="Save current scene">Save Scene</n-button>
            <n-button @click="restartEngine" secondary title="Restart Engine sidecar">Restart</n-button>
            <n-button @click="stopEngine" type="error" secondary title="Stop Engine sidecar">Stop</n-button>
          </n-button-group>
          <n-tag :type="isConnected ? 'success' : 'error'" size="small" round>
            <template #icon>
              <div :class="['status-dot', isConnected ? 'connected' : '']"></div>
            </template>
            {{ isConnected ? 'Connected' : 'Disconnected' }}
          </n-tag>
        </n-space>
      </div>
    </n-layout-header>

    <n-layout-content content-style="padding: 0;" class="dock-container">
      <dockview-vue
        class="dockview-theme-abyssal"
        @ready="onDockviewReady"
        :components="dockviewComponents"
      >
      </dockview-vue>
    </n-layout-content>

    <n-layout-footer bordered class="editor-footer">
      <n-space justify="space-between" align="center" style="width: 100%; height: 100%; padding: 0 10px;">
        <n-text depth="3" style="font-size: 11px;">Ready</n-text>
        <n-text depth="3" class="flex-grow" style="font-size: 11px;">Console: {{ lastMessage || 'No engine output' }}</n-text>
        <n-text depth="3" style="font-size: 11px;">FPS: 60</n-text>
      </n-space>
    </n-layout-footer>
  </n-layout>
</template>

<script setup>
import { ref, onMounted, onUnmounted, shallowRef, markRaw } from 'vue'
import { 
  NLayout, NLayoutHeader, NLayoutContent, NLayoutFooter,
  NMenu, NButton, NButtonGroup, NTag, NSpace, NText, useMessage
} from 'naive-ui'
import { DockviewVue } from 'dockview-vue'
import ViewportPanel from './ViewportPanel.vue'
import HierarchyPanel from './HierarchyPanel.vue'
import PropertyPanel from './PropertyPanel.vue'
import AssetPanel from './AssetPanel.vue'

import 'dockview-vue/dist/styles/dockview.css'

// Menu Options
const menuOptions = [
  { label: 'File', key: 'file' },
  { label: 'Edit', key: 'edit' },
  { label: 'View', key: 'view' },
  { label: 'Engine', key: 'engine' },
  { label: 'Help', key: 'help' }
]

// State
const isConnected = ref(false)
const sharedHandle = ref(null)
const lastMessage = ref('')
const entities = ref([])
const selectedEntityId = ref(null)
const selectedEntity = ref(null)

const { ipcRenderer } = require('electron')
const message = useMessage()

// Dockview
const dockviewApi = shallowRef()

const setupIpc = () => {
  ipcRenderer.on('engine-connected', (event, connected) => {
    isConnected.value = connected
    if (connected) {
      lastMessage.value = 'Engine connected'
      ipcRenderer.send('engine-message', { type: 'GET_SCENE' })
    } else {
      lastMessage.value = 'Engine disconnected'
      sharedHandle.value = null
      updatePanelParams('viewport_panel', { sharedHandle: null })
    }
  })

  ipcRenderer.on('viewport-ready', (event, info) => {
    lastMessage.value = 'Viewport ready'
    if (info.sharedHandle) {
      sharedHandle.value = BigInt(info.sharedHandle)
      updatePanelParams('viewport_panel', { sharedHandle: sharedHandle.value })
    }
  })

  ipcRenderer.on('engine-message', (event, msg) => {
    lastMessage.value = `Received: ${msg.type}`
    if (msg.type === 'SCENE_DATA') {
      entities.value = msg.entities
      updatePanelParams('hierarchy_panel', { entities: entities.value })
    } else if (msg.type === 'ENTITY_DETAILS') {
      selectedEntity.value = msg.details
      updatePanelParams('properties_panel', { selectedEntity: selectedEntity.value })
    }
  })
}

const onUpdateProperty = (data) => {
  ipcRenderer.send('engine-message', {
    type: 'UPDATE_ENTITY_PROPERTY',
    ...data
  })
}

// Dockview components definition for v5.2.0 API
const dockviewComponents = {
  viewport: markRaw(ViewportPanel),
  hierarchy: markRaw(HierarchyPanel),
  properties: markRaw(PropertyPanel),
  assets: markRaw(AssetPanel)
}

const onDockviewReady = (event) => {
  dockviewApi.value = event.api

  // Create default layout
  const hierarchyPane = event.api.addPanel({
    id: 'hierarchy_panel',
    component: 'hierarchy',
    title: 'Hierarchy',
    params: { 
      entities: entities.value, 
      selectedEntityId: selectedEntityId.value,
      onSelectEntity: selectEntity 
    },
    position: { direction: 'left', width: 300 }
  })

  const viewportPane = event.api.addPanel({
    id: 'viewport_panel',
    component: 'viewport',
    title: 'Viewport',
    params: { sharedHandle: sharedHandle.value }
  })

  const propertiesPane = event.api.addPanel({
    id: 'properties_panel',
    component: 'properties',
    title: 'Properties',
    params: { 
      selectedEntity: selectedEntity.value,
      onUpdateProperty: onUpdateProperty 
    },
    position: { direction: 'right', referencePanel: viewportPane, width: 350 }
  })

  const assetsPane = event.api.addPanel({
    id: 'assets_panel',
    component: 'assets',
    title: 'Assets',
    position: { direction: 'below', referencePanel: viewportPane, height: 250 }
  })
}

const updatePanelParams = (id, params) => {
  if (dockviewApi.value) {
    const panel = dockviewApi.value.getPanel(id)
    if (panel) {
      panel.update({ params: { ...panel.params, ...params } })
    }
  }
}

const selectEntity = (id) => {
  selectedEntityId.value = id
  updatePanelParams('hierarchy_panel', { selectedEntityId: id })
  ipcRenderer.send('engine-message', { type: 'GET_ENTITY_DETAILS', id: id })
}

const loadScene = (path) => {
  ipcRenderer.send('engine-message', { type: 'LOAD_SCENE', path: path })
}

const saveScene = () => {
  ipcRenderer.send('engine-message', { type: 'SAVE_SCENE' })
  message.success('Scene save request sent to engine')
}

const restartEngine = () => {
  ipcRenderer.send('engine-restart')
  message.info('Engine restart requested')
}

const stopEngine = () => {
  ipcRenderer.send('engine-stop')
  message.warning('Engine stop requested')
}

onMounted(() => {
  setupIpc()
  window.addEventListener('load-scene', (e) => loadScene(e.detail))
})

onUnmounted(() => {
  ipcRenderer.removeAllListeners('engine-connected')
  ipcRenderer.removeAllListeners('viewport-ready')
  ipcRenderer.removeAllListeners('engine-message')
})
</script>

<style scoped>
.editor-header {
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 10px;
  user-select: none;
}

.header-left { display: flex; align-items: center; gap: 20px; }
.logo { font-weight: bold; font-size: 13px; color: #fff; }
.menu-bar { width: 300px; height: 100%; border: none !important; }

.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #ff4d4f;
  margin-right: 4px;
}
.status-dot.connected {
  background: #18a058;
  box-shadow: 0 0 4px #18a058;
}

.dock-container {
  flex: 1;
  position: relative;
}

.editor-footer {
  height: 24px;
}

.flex-grow { flex: 1; text-align: center; }
</style>
