<script setup>
import { ref, computed, onMounted } from 'vue'
import {
  NList,
  NListItem,
  NScrollbar,
  NText,
  NSpace,
  NIcon,
  NEmpty,
  NButton,
  useMessage,
} from 'naive-ui'
import { LayersOutline, RefreshOutline, CheckmarkCircle } from '@vicons/ionicons5'
import { connectionStore } from '../store/connectionStore'

const fs = window.require ? window.require('fs') : null
const path = window.require ? window.require('path') : null
const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }
const message = useMessage()

// Scenes live at <repo>/Data/ExampleProject/Scenes/. Editor source root is
// Source/Editor-Next, so the data dir is two levels up + "Data". Engine load
// paths are relative to Bin/../Data/, i.e. "ExampleProject/Scenes/<name>".
const SCENES_RELATIVE_TO_DATA = 'ExampleProject/Scenes'
const dataRoot = path
  ? path.resolve(__dirname, '../../../../Data')
  : ''
const scenesDir = path ? path.join(dataRoot, SCENES_RELATIVE_TO_DATA) : ''

const scenes = ref([])
const lastLoaded = ref(null)
const loadError = ref(null)

const refresh = () => {
  if (!fs) {
    loadError.value = 'Filesystem unavailable (running outside Electron)'
    scenes.value = []
    return
  }
  try {
    const entries = fs.readdirSync(scenesDir, { withFileTypes: true })
    scenes.value = entries
      .filter((e) => e.isFile() && e.name.endsWith('.InnoScene'))
      .map((e) => ({
        name: e.name.replace(/\.InnoScene$/, ''),
        fileName: e.name,
        enginePath: `${SCENES_RELATIVE_TO_DATA}/${e.name}`,
      }))
      .sort((a, b) => a.name.localeCompare(b.name))
    loadError.value = null
  } catch (err) {
    loadError.value = `Could not read ${scenesDir}: ${err.message}`
    scenes.value = []
  }
}

const loadScene = (scene) => {
  if (!connectionStore.isConnected) {
    message.warning('Engine not connected')
    return
  }
  if (!ipcRenderer) {
    message.error('IPC unavailable')
    return
  }
  ipcRenderer.send('engine-message', { type: 'LOAD_SCENE', path: scene.enginePath })
  lastLoaded.value = scene.name
  message.success(`Loading ${scene.name}…`)
}

const isActive = (scene) => lastLoaded.value === scene.name

onMounted(refresh)
</script>

<template>
  <div class="scene-panel" data-test="scene-panel">
    <div class="scene-toolbar">
      <n-text depth="3" style="font-size: 12px">
        {{ scenesDir }}
      </n-text>
      <n-button size="tiny" quaternary @click="refresh" data-test="scene-refresh">
        <template #icon>
          <n-icon><refresh-outline /></n-icon>
        </template>
        Refresh
      </n-button>
    </div>

    <div v-if="loadError" class="empty-container">
      <n-empty :description="loadError" size="small" />
    </div>

    <n-scrollbar v-else class="scene-list">
      <n-empty v-if="!scenes.length" description="No .InnoScene files found" size="small" />
      <n-list v-else hoverable clickable size="small" :show-divider="false">
        <n-list-item
          v-for="scene in scenes"
          :key="scene.enginePath"
          class="scene-item"
          :data-test="`scene-row-${scene.name}`"
          @click="loadScene(scene)"
        >
          <n-space align="center" :size="8" justify="space-between" style="width: 100%">
            <n-space align="center" :size="8">
              <n-icon size="16">
                <layers-outline />
              </n-icon>
              <n-text :strong="isActive(scene)">{{ scene.name }}</n-text>
            </n-space>
            <n-icon v-if="isActive(scene)" size="14" color="var(--ctp-green)">
              <checkmark-circle />
            </n-icon>
          </n-space>
        </n-list-item>
      </n-list>
    </n-scrollbar>
  </div>
</template>

<style scoped>
.scene-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.scene-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--ctp-surface0);
}

.empty-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.scene-list {
  flex: 1;
}

.scene-item {
  cursor: pointer;
}
</style>
