<template>
  <div class="asset-panel">
    <div class="panel-header">Asset Browser</div>
    <div class="toolbar">
      <button @click="navigateUp" :disabled="isRoot">⬆️</button>
      <div class="current-path">{{ currentPath }}</div>
    </div>
    <div class="asset-grid">
      <div 
        v-for="item in items" 
        :key="item.name"
        class="asset-item"
        :class="{ folder: item.isDir }"
        @dblclick="onItemDblClick(item)"
      >
        <div class="icon">{{ item.isDir ? '📁' : '📄' }}</div>
        <div class="name">{{ item.name }}</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'

const currentPath = ref('')
const items = ref([])
const isRoot = computed(() => currentPath.value === '' || currentPath.value === '.')

let fs, path, baseDir

onMounted(() => {
  if (window.require) {
    fs = window.require('fs')
    path = window.require('path')
    // Data is 2 levels up from Source/Editor-Next (where Electron runs from in dev)
    // Or relative to process.cwd()
    baseDir = path.join(window.process.cwd(), '../../Data')
    loadDirectory('')
  }
})

const loadDirectory = (relPath) => {
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

const onItemDblClick = (item) => {
  if (item.isDir) {
    loadDirectory(path.join(currentPath.value, item.name))
  }
}
</script>

<style scoped>
.asset-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #252526;
  color: #ccc;
}

.panel-header {
  padding: 8px 12px;
  background: #2d2d2d;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid #333;
}

.toolbar {
  padding: 4px 8px;
  background: #333;
  display: flex;
  align-items: center;
  gap: 8px;
  border-bottom: 1px solid #222;
}

.toolbar button {
  background: transparent;
  border: none;
  color: #fff;
  cursor: pointer;
  opacity: 0.7;
}

.toolbar button:hover { opacity: 1; }
.toolbar button:disabled { opacity: 0.2; cursor: default; }

.current-path {
  font-size: 11px;
  opacity: 0.6;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.asset-grid {
  flex: 1;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
  gap: 10px;
  padding: 15px;
  overflow-y: auto;
}

.asset-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  padding: 8px;
  border-radius: 4px;
  cursor: pointer;
  user-select: none;
}

.asset-item:hover {
  background: #37373d;
}

.asset-item .icon {
  font-size: 32px;
  margin-bottom: 4px;
}

.asset-item .name {
  font-size: 11px;
  word-break: break-all;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.asset-item.folder .icon { color: #dcb67a; }
</style>
