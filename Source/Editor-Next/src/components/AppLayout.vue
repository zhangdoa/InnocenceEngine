<template>
  <div class="editor-shell">
    <editor-header />

    <main class="dock-container">
      <dockview-vue
        class="dockview-theme-ctp"
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
import { DockviewVue } from 'dockview-vue'
import EditorHeader from './layout/EditorHeader.vue'
import EditorFooter from './layout/EditorFooter.vue'
import ImportModal from './layout/ImportModal.vue'
import { panelStore } from '../store/panelStore'

// Importing the store barrel wires every store's engine-connected
// subscription before the first paint and exposes window.__innoStores for
// Playwright harnesses to drive.
import '../store'

import 'dockview-vue/dist/styles/dockview.css'

// Declarative panel descriptors. panelStore registers them up front so the
// Window menu can list (and re-open) them even before dockview is ready;
// the actual addPanel calls happen in onDockviewReady once the API exists.
// No viewport panel — the engine renders into its own native window (see
// project CLAUDE.md design notes). The editor is tooling only; the game
// view lives in Main.exe's OS window alongside this one.
panelStore.register({
  id: 'hierarchy_panel',
  component: 'hierarchy',
  title: 'Outliner',
  position: { direction: 'within', referencePanel: null },
})
panelStore.register({
  id: 'properties_panel',
  component: 'properties',
  title: 'Inspector',
  position: { direction: 'right', referencePanel: 'hierarchy_panel', width: 400 },
})
panelStore.register({
  id: 'assets_panel',
  component: 'assets',
  title: 'Workspace',
  position: { direction: 'below', referencePanel: 'hierarchy_panel', height: 300 },
})
panelStore.register({
  id: 'render_toggles_panel',
  component: 'render-toggles',
  title: 'Render Toggles',
  position: { direction: 'below', referencePanel: 'hierarchy_panel', height: 240 },
})
panelStore.register({
  id: 'rt_debugger_panel',
  component: 'rt-debugger',
  title: 'RT Debugger',
  position: { direction: 'below', referencePanel: 'render_toggles_panel', height: 240 },
})
panelStore.register({
  id: 'task_debugger_panel',
  component: 'task-debugger',
  title: 'Task Debugger',
  position: { direction: 'below', referencePanel: 'rt_debugger_panel', height: 280 },
})

const onDockviewReady = (event) => {
  panelStore.setApi(event.api)
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
