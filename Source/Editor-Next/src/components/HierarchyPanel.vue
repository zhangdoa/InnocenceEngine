<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { NList, NListItem, NInput, NScrollbar, NText, NSpace, NIcon, NEmpty } from 'naive-ui'
import { CubeOutline, SearchOutline } from '@vicons/ionicons5'
import { sceneStore } from '../store/sceneStore'
import { connectionStore } from '../store/connectionStore'

onMounted(() => {
  console.log('HierarchyPanel mounted');
})

const searchQuery = ref('')

const filteredEntities = computed(() => {
  const entities = sceneStore.entities || []
  if (!searchQuery.value) return entities
  return entities.filter(e => 
    e.name.toLowerCase().includes(searchQuery.value.toLowerCase())
  )
})

const selectEntity = (id) => {
  sceneStore.selectEntity(id)
}
</script>

<template>
  <div class="hierarchy-panel">
    <div class="search-bar">
      <n-input size="small" placeholder="Search outliner..." v-model:value="searchQuery" clearable>
        <template #prefix>
          <n-icon><search-outline /></n-icon>
        </template>
      </n-input>
    </div>

    <div v-if="!connectionStore.isConnected && sceneStore.entities.length === 0" class="empty-container">
      <n-empty description="System Offline" size="small" />
    </div>

    <n-scrollbar v-else class="entity-list">
      <n-list hoverable clickable size="small" :show-divider="false">
        <n-list-item 
          v-for="entity in filteredEntities" 
          :key="entity.id"
          :class="['entity-item', sceneStore.selectedEntityId === entity.id ? 'selected' : '']"
          @click="selectEntity(entity.id)"
        >
          <n-space align="center" :size="8">
            <n-icon size="16">
              <cube-outline />
            </n-icon>
            <n-text :strong="sceneStore.selectedEntityId === entity.id">
              {{ entity.name }}
            </n-text>
          </n-space>
        </n-list-item>
      </n-list>
    </n-scrollbar>
  </div>
</template>

<style scoped>
.hierarchy-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.search-bar {
  padding: 12px;
}

.empty-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.entity-list {
  flex: 1;
}

.entity-item.selected {
  position: relative;
}
</style>
