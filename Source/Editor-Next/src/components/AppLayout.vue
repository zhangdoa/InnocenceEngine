<template>
  <div class="editor-shell">
    <editor-header />

    <main class="dock-container">
      <dockview-vue
        class="dockview-theme-dark"
        style="width: 100%; height: 100%;"
        @ready="onDockviewReady"
      >
      </dockview-vue>
    </main>

    <import-modal />
    <editor-footer />
  </div>
</template>

<script setup>
import { shallowRef } from 'vue'
import { DockviewVue } from 'dockview-vue'
import EditorHeader from './layout/EditorHeader.vue'
import EditorFooter from './layout/EditorFooter.vue'
import ImportModal from './layout/ImportModal.vue'
import { useIpc } from '../composables/useIpc'

import 'dockview-vue/dist/styles/dockview.css'

// Initialize IPC message routing
useIpc()

const dockviewApi = shallowRef()

const onDockviewReady = (event) => {
  dockviewApi.value = event.api
  
  const viewportPane = event.api.addPanel({
    id: 'viewport_panel',
    component: 'viewport',
    title: 'Viewport',
    position: { direction: 'within', referencePanel: null }
  })

  const hierarchyPane = event.api.addPanel({
    id: 'hierarchy_panel',
    component: 'hierarchy',
    title: 'Outliner',
    position: { direction: 'left', referencePanel: viewportPane, width: 300 }
  })

  event.api.addPanel({
    id: 'properties_panel',
    component: 'properties',
    title: 'Inspector',
    position: { direction: 'right', referencePanel: viewportPane, width: 400 }
  })

  event.api.addPanel({
    id: 'assets_panel',
    component: 'assets',
    title: 'Workspace',
    position: { direction: 'below', referencePanel: viewportPane, height: 300 }
  })

  event.api.addPanel({
    id: 'scenes_panel',
    component: 'scenes',
    title: 'Scenes',
    position: { direction: 'below', referencePanel: hierarchyPane, height: 220 }
  })
}
</script>

<style scoped>
.editor-shell {
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  background: var(--ctp-base);
  color: var(--ctp-text);
  transition: all 0.3s ease;
}

.dock-container {
  flex: 1;
  position: relative;
  overflow: hidden;
}
</style>
