<template>
  <div class="asset-panel">
    <div class="panel-header">
      <n-text depth="3" strong>Asset Browser</n-text>
    </div>
    
    <div class="toolbar">
      <n-space align="center" :size="12">
        <n-button size="tiny" quaternary @click="navigateUp" :disabled="isRoot">
          <template #icon><span>⬆️</span></template>
        </n-button>
        <n-breadcrumb separator=">">
          <n-breadcrumb-item>Data</n-breadcrumb-item>
          <n-breadcrumb-item v-for="(part, i) in pathParts" :key="i">{{ part }}</n-breadcrumb-item>
        </n-breadcrumb>
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
              <span v-if="item.isDir" style="font-size: 32px;">📁</span>
              <span v-else-if="item.name.endsWith('.InnoScene')" style="font-size: 32px;">🌍</span>
              <span v-else style="font-size: 32px;">📄</span>
            </div>
            <n-text class="asset-name">{{ item.name }}</n-text>
          </div>
        </n-grid-item>
      </n-grid>
    </n-scrollbar>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { 
  NGrid, NGridItem, NBreadcrumb, NBreadcrumbItem, NButton, 
  NSpace, NText, NScrollbar 
} from 'naive-ui'

const currentPath = ref('')
const items = ref([])
const isRoot = computed(() => currentPath.value === '' || currentPath.value === '.')
const pathParts = computed(() => currentPath.value.split(/[\\\/]/).filter(p => p))

let fs, path, baseDir

onMounted(() => {
  if (window.require) {
    fs = window.require('fs')
    path = window.require('path')
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
  } else if (item.name.endsWith('.InnoScene')) {
    if (window.require) {
      const relPath = path.join(currentPath.value, item.name)
      window.dispatchEvent(new CustomEvent('load-scene', { detail: relPath }))
    }
  }
}
</script>

<style scoped>
.asset-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #18181c;
}

.panel-header {
  padding: 8px 12px;
  background: #262629;
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid #333;
}

.toolbar {
  padding: 6px 12px;
  background: #1e1e22;
  border-bottom: 1px solid #222;
}

.asset-content {
  flex: 1;
  padding: 16px;
}

.asset-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 8px;
  border-radius: 4px;
  cursor: pointer;
  transition: background 0.2s ease;
}

.asset-item:hover {
  background: #2a2d2e;
}

.icon-wrapper {
  margin-bottom: 8px;
  filter: drop-shadow(0 2px 4px rgba(0,0,0,0.3));
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
