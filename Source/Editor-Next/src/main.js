import { createApp, h } from 'vue'
import App from './App.vue'

import HierarchyPanel from './components/HierarchyPanel.vue'
import PropertyPanel from './components/PropertyPanel.vue'
import AssetPanel from './components/AssetPanel.vue'
import RenderTogglesPanel from './components/RenderTogglesPanel.vue'
import RenderTargetDebuggerPanel from './components/RenderTargetDebuggerPanel.vue'
import TaskDebuggerPanel from './components/TaskDebuggerPanel.vue'
import ThemedPanelHost from './components/ThemedPanelHost.vue'

const app = createApp(App)

// Wrap every dockview-mounted panel in ThemedPanelHost so Naive UI's
// theme / message / dialog providers reach widgets inside the panel.
// Without this, dockview-vue's mountVueComponent only carries direct
// parent provides across the boundary and Naive widgets render with
// their default theme — symptom: white input bgs, green focus rings
// even in mocha.
const hosted = (Comp) => ({
  name: (Comp.__name || 'Panel') + 'Hosted',
  render() { return h(ThemedPanelHost, null, { default: () => h(Comp) }) },
})

app.component('hierarchy',             hosted(HierarchyPanel))
app.component('properties',            hosted(PropertyPanel))
app.component('assets',                hosted(AssetPanel))
app.component('render-toggles',        hosted(RenderTogglesPanel))
app.component('render-target-debugger', hosted(RenderTargetDebuggerPanel))
app.component('task-debugger',         hosted(TaskDebuggerPanel))

app.mount('#app')
