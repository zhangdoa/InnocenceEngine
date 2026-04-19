<template>
  <div class="asset-panel">
    <div class="toolbar">
      <n-space align="center" justify="space-between" style="width: 100%;">
        <n-space align="center" :size="12">
          <n-button size="tiny" quaternary @click="navigateUp" :disabled="isRoot">
            <template #icon><n-icon><arrow-up-outline /></n-icon></template>
          </n-button>
          <n-breadcrumb separator=">">
            <n-breadcrumb-item>Data</n-breadcrumb-item>
            <n-breadcrumb-item v-for="(part, i) in pathParts" :key="i">{{ part }}</n-breadcrumb-item>
          </n-breadcrumb>
        </n-space>
        
        <n-button-group size="small">
          <n-button secondary type="primary" @click="triggerImport" data-test="import-files">
            <template #icon><n-icon><add-outline /></n-icon></template>
            Import files
          </n-button>
          <n-button secondary @click="triggerImportFolder" data-test="import-folder">
            <template #icon><n-icon><folder-outline /></n-icon></template>
            Import folder
          </n-button>
        </n-button-group>
      </n-space>
    </div>

    <n-scrollbar class="asset-content">
      <n-grid :cols="10" :x-gap="12" :y-gap="12" item-responsive responsive="screen">
        <n-grid-item v-for="item in items" :key="item.name" span="2 m:1">
          <div 
            class="asset-item" 
            :title="item.name"
            @dblclick="onItemDblClick(item)"
          >
            <div class="icon-wrapper">
              <n-icon size="32">
                <folder-outline v-if="item.isDir" />
                <planet-outline v-else-if="item.name.endsWith('.InnoScene')" />
                <document-outline v-else />
              </n-icon>
            </div>
            <n-text class="asset-name">{{ item.name }}</n-text>
          </div>
        </n-grid-item>
      </n-grid>
    </n-scrollbar>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import {
  NGrid, NGridItem, NBreadcrumb, NBreadcrumbItem, NButton, NButtonGroup,
  NSpace, NText, NScrollbar, NIcon, useMessage
} from 'naive-ui'
import { FolderOutline, DocumentOutline, PlanetOutline, ArrowUpOutline, AddOutline } from '@vicons/ionicons5'
import { request } from '../composables/useIpc'
import { connectionStore } from '../store/connectionStore'

const message = useMessage()

const currentPath = ref('')
const items = ref([])
const isRoot = computed(() => currentPath.value === '' || currentPath.value === '.')
const pathParts = computed(() => currentPath.value.split(/[\\\/]/).filter(p => p))

let fs, path, baseDir

const { ipcRenderer } = window.require ? window.require('electron') : { ipcRenderer: null }

const onRefreshAssets = () => loadDirectory(currentPath.value)

onMounted(() => {
  if (window.require) {
    fs = window.require('fs')
    path = window.require('path')
    // __dirname (the editor source dir) is the only stable anchor for
    // resolving repo-relative paths from any launcher's cwd.
    baseDir = path.resolve(__dirname, '../../../../Data')
    loadDirectory('')
  }
  window.addEventListener('refresh-assets', onRefreshAssets)
})

onUnmounted(() => {
  window.removeEventListener('refresh-assets', onRefreshAssets)
})

const loadDirectory = (relPath) => {
  if (!fs) return;
  const fullPath = path.join(baseDir, relPath)
  if (!fs.existsSync(fullPath)) return

  const files = fs.readdirSync(fullPath)
  items.value = files.map(name => {
    const stats = fs.statSync(path.join(fullPath, name))
    return {
      name,
      isDir: stats.isDirectory(),
      size: stats.size
    }
  }).sort((a, b) => (b.isDir === a.isDir ? a.name.localeCompare(b.name) : b.isDir ? 1 : -1))
  
  currentPath.value = relPath
}

const navigateUp = () => {
  const parent = path.dirname(currentPath.value)
  loadDirectory(parent === '.' ? '' : parent)
}

const onItemDblClick = async (item) => {
  if (item.isDir) {
    loadDirectory(path.join(currentPath.value, item.name))
    return
  }
  if (!item.name.endsWith('.InnoScene')) return
  if (!connectionStore.isConnected) {
    message.warning('Engine offline; scene load ignored')
    return
  }
  const relPath = path.join(currentPath.value, item.name)
  try {
    await request('LOAD_SCENE', { path: relPath })
    message.info(`Loading ${relPath}…`)
  } catch (e) {
    message.error(`Load failed: ${e.message}`)
  }
}

const triggerImport = () => {
  if (ipcRenderer) ipcRenderer.send('select-files')
}

const triggerImportFolder = () => {
  if (ipcRenderer) ipcRenderer.send('select-folder')
}
</script>

<style scoped>
.asset-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.toolbar {
  padding: 8px 16px;
  border-bottom: 1px solid var(--ctp-surface1);
}

.asset-content {
  flex: 1;
  padding: 16px;
}

.asset-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 12px 8px;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.asset-item:hover {
  background: var(--ctp-surface0);
}

.icon-wrapper {
  margin-bottom: 8px;
}

.asset-name {
  font-size: 11px;
  text-align: center;
  word-break: break-all;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
  max-width: 100%;
}
</style>
