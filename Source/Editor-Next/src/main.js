import { createApp } from 'vue'
import App from './App.vue'

import ViewportPanel from './components/ViewportPanel.vue'
import HierarchyPanel from './components/HierarchyPanel.vue'
import PropertyPanel from './components/PropertyPanel.vue'
import AssetPanel from './components/AssetPanel.vue'

const app = createApp(App)

app.component('viewport', ViewportPanel)
app.component('hierarchy', HierarchyPanel)
app.component('properties', PropertyPanel)
app.component('assets', AssetPanel)

app.mount('#app')
