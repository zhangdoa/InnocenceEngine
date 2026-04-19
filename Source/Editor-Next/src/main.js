import { createApp } from 'vue'
import App from './App.vue'

import HierarchyPanel from './components/HierarchyPanel.vue'
import PropertyPanel from './components/PropertyPanel.vue'
import AssetPanel from './components/AssetPanel.vue'
import RenderTogglesPanel from './components/RenderTogglesPanel.vue'
import RTDebuggerPanel from './components/RTDebuggerPanel.vue'
import TaskDebuggerPanel from './components/TaskDebuggerPanel.vue'

const app = createApp(App)

app.component('hierarchy', HierarchyPanel)
app.component('properties', PropertyPanel)
app.component('assets', AssetPanel)
app.component('render-toggles', RenderTogglesPanel)
app.component('rt-debugger', RTDebuggerPanel)
app.component('task-debugger', TaskDebuggerPanel)

app.mount('#app')
