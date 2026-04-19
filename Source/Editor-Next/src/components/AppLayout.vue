<template>
  <div class="editor-shell">
    <editor-header />

    <main class="dock-container">
      <dockview-vue
        :theme="ctpTheme"
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

// dockview-core defaults to `themeAbyss` when no `theme` prop is passed —
// it calls `setClassNames(themeAbyss.className)` on an internal node whose
// CSS-var definitions shadow anything we put on the outer wrapper. Pass
// our own theme descriptor so that internal node gets `dockview-theme-ctp`
// instead, and the port under src/theme/dockview-ctp.css actually applies.
const ctpTheme = { name: 'ctp', className: 'dockview-theme-ctp' }

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
//
// Layout: left column = functional panels (render toggles, RT debugger,
// task debugger), centre = workspace/assets, right column = outliner
// over inspector. The first panel registered anchors everything else,
// so assets (the centre) is registered first.
panelStore.register({
  id: 'assets_panel',
  component: 'assets',
  title: 'Workspace',
  position: { direction: 'within', referencePanel: null },
})

// Left column — stacked functional panels.
panelStore.register({
  id: 'render_toggles_panel',
  component: 'render-toggles',
  title: 'Render Toggles',
  position: { direction: 'left', referencePanel: 'assets_panel', width: 280 },
})
panelStore.register({
  id: 'render_target_debugger_panel',
  component: 'render-target-debugger',
  title: 'Render Target Debugger',
  position: { direction: 'below', referencePanel: 'render_toggles_panel', height: 260 },
})
panelStore.register({
  id: 'task_debugger_panel',
  component: 'task-debugger',
  title: 'Task Debugger',
  position: { direction: 'below', referencePanel: 'render_target_debugger_panel', height: 260 },
})

// Right column — outliner over inspector.
panelStore.register({
  id: 'hierarchy_panel',
  component: 'hierarchy',
  title: 'Outliner',
  position: { direction: 'right', referencePanel: 'assets_panel', width: 340 },
})
panelStore.register({
  id: 'properties_panel',
  component: 'properties',
  title: 'Inspector',
  position: { direction: 'below', referencePanel: 'hierarchy_panel', height: 420 },
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
