<template>
  <div class="editor-shell">
    <header class="editor-header">
      <div class="header-left">
        <div class="logo">InnocenceEngine</div>
        <nav class="menu-bar">
          <span>File</span>
          <span>Edit</span>
          <span>View</span>
          <span>Engine</span>
          <span>Help</span>
        </nav>
      </div>
      <div class="header-right">
        <div class="engine-status" :class="{ connected: isConnected }">
          {{ isConnected ? '● Connected' : '○ Disconnected' }}
        </div>
      </div>
    </header>

    <div class="dock-container">
      <dockview-vue
        class="dockview-theme-abyssal"
        @ready="onDockviewReady"
      >
      </dockview-vue>
    </div>

    <footer class="editor-footer">
      <div class="footer-item">Ready</div>
      <div class="footer-item flex-grow">Console: {{ lastMessage || 'No engine output' }}</div>
      <div class="footer-item">FPS: 60</div>
    </footer>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, shallowRef, markRaw } from 'vue'
import { DockviewVue, DockviewComponent } from 'dockview-vue'
import ViewportPanel from './components/ViewportPanel.vue'
import HierarchyPanel from './components/HierarchyPanel.vue'
import PropertyPanel from './components/PropertyPanel.vue'
import AssetPanel from './components/AssetPanel.vue'

import 'dockview-vue/dist/styles/dockview.css'

// State
const isConnected = ref(false)
const sharedHandle = ref(null)
const lastMessage = ref('')
const entities = ref([
  { id: 1, name: 'Main Camera' },
  { id: 2, name: 'Directional Light' },
  { id: 3, name: 'Sponza Palace' }
])
const selectedEntityId = ref(null)
const selectedEntity = ref(null)

let socket = null

// Dockview
const dockviewApi = shallowRef()

const onDockviewReady = (event) => {
  dockviewApi.value = event.api

  // Register components
  event.api.registerComponent('viewport', markRaw(ViewportPanel))
  event.api.registerComponent('hierarchy', markRaw(HierarchyPanel))
  event.api.registerComponent('properties', markRaw(PropertyPanel))
  event.api.registerComponent('assets', markRaw(AssetPanel))

  // Create default layout
  const hierarchyPane = event.api.addPanel({
    id: 'hierarchy_panel',
    component: 'hierarchy',
    title: 'Hierarchy',
    params: { entities: entities.value, selectedEntityId: selectedEntityId.value },
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
    params: { selectedEntity: selectedEntity.value },
    position: { direction: 'right', referencePanel: viewportPane, width: 350 }
  })

  const assetsPane = event.api.addPanel({
    id: 'assets_panel',
    component: 'assets',
    title: 'Assets',
    position: { direction: 'below', referencePanel: viewportPane, height: 250 }
  })
}

const connect = () => {
  console.log('Connecting to Engine...')
  socket = new WebSocket('ws://localhost:8081')

  socket.onopen = () => {
    isConnected.value = true
    socket.send(JSON.stringify({ type: 'HELO' }))
  }

  socket.onmessage = (event) => {
    lastMessage.value = event.data
    try {
      const msg = JSON.parse(event.data)
      if (msg.type === 'HELLO_REPLY') {
        sharedHandle.value = BigInt(msg.sharedHandle)
        updatePanelParams('viewport_panel', { sharedHandle: sharedHandle.value })
        // Request scene data after handshake
        socket.send(JSON.stringify({ type: 'GET_SCENE' }))
      } else if (msg.type === 'SCENE_DATA') {
        entities.value = msg.entities
        updatePanelParams('hierarchy_panel', { entities: entities.value })
      } else if (msg.type === 'ENTITY_DETAILS') {
        selectedEntity.value = msg.details
        updatePanelParams('properties_panel', { selectedEntity: selectedEntity.value })
      }
    } catch (e) {
      console.error('Failed to parse message:', e)
    }
  }

  socket.onclose = () => {
    isConnected.value = false
    sharedHandle.value = null
    updatePanelParams('viewport_panel', { sharedHandle: null })
    setTimeout(connect, 2000)
  }
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
  selectedEntity.value = entities.value.find(e => e.id === id)
  updatePanelParams('hierarchy_panel', { selectedEntityId: id })
  updatePanelParams('properties_panel', { selectedEntity: selectedEntity.value })
}

onMounted(() => {
  connect()
})

onUnmounted(() => {
  if (socket) socket.close()
})
</script>

<style>
/* Global abyssal theme overrides for dockview */
.dockview-theme-abyssal {
  --dv-pane-background-color: #1e1e1e;
  --dv-tabs-and-actions-container-background-color: #252526;
  --dv-activegroup-visiblepanel-tab-background-color: #1e1e1e;
  --dv-inactivegroup-visiblepanel-tab-background-color: #2d2d2d;
  --dv-tab-text-color: #aaa;
  --dv-active-tab-text-color: #fff;
  --dv-separator-color: #333;
}

body {
  margin: 0;
  padding: 0;
  overflow: hidden;
  background: #1e1e1e;
  color: #ccc;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
}

.editor-shell {
  display: flex;
  flex-direction: column;
  height: 100vh;
}

.editor-header {
  height: 35px;
  background: #3c3c3c;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 10px;
  border-bottom: 1px solid #222;
  user-select: none;
}

.header-left { display: flex; align-items: center; gap: 20px; }
.logo { font-weight: bold; font-size: 12px; color: #fff; }
.menu-bar { display: flex; gap: 15px; font-size: 12px; }
.menu-bar span { cursor: pointer; opacity: 0.8; }
.menu-bar span:hover { opacity: 1; }

.engine-status { font-size: 11px; opacity: 0.6; }
.engine-status.connected { color: #42b983; opacity: 1; }

.dock-container {
  flex: 1;
  position: relative;
}

.editor-footer {
  height: 22px;
  background: #007acc;
  color: #fff;
  display: flex;
  align-items: center;
  padding: 0 10px;
  font-size: 11px;
}

.footer-item { padding: 0 10px; border-right: 1px solid rgba(255,255,255,0.2); }
.footer-item:last-child { border-right: none; }
.flex-grow { flex: 1; }
</style>
