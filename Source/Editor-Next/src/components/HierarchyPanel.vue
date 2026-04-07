<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { NList, NListItem, NInput, NScrollbar, NText, NSpace, NIcon, NEmpty } from 'naive-ui'
import { CubeOutline, SearchOutline } from '@vicons/ionicons5'
import { editorState } from '../store'

onMounted(() => {
  console.log('HierarchyPanel mounted');
})

watch(() => editorState.entities, (newEntities) => {
  console.log(`HierarchyPanel: entities updated, count: ${newEntities?.length || 0}`);
}, { deep: true, immediate: true })

const searchQuery = ref('')

const filteredEntities = computed(() => {
  const entities = editorState.entities || []
  if (!searchQuery.value) return entities
  return entities.filter(e => 
    e.name.toLowerCase().includes(searchQuery.value.toLowerCase())
  )
})

const selectEntity = (id) => {
  editorState.selectEntity(id)
}
</script>

<template>
  <div class="hierarchy-panel">
    <div class="panel-header">
      <n-text depth="3" strong>Scene Outliner</n-text>
    </div>
    
    <div class="search-bar">
      <n-input size="small" placeholder="Search entities..." v-model:value="searchQuery" clearable>
        <template #prefix>
          <n-icon><search-outline /></n-icon>
        </template>
      </n-input>
    </div>

    <div v-if="!editorState.isConnected && editorState.entities.length === 0" class="empty-container">
      <n-empty description="System Offline" size="small" />
    </div>

    <n-scrollbar v-else class="entity-list">
      <n-list hoverable clickable size="small" :show-divider="false">
        <n-list-item 
          v-for="entity in filteredEntities" 
          :key="entity.id"
          :class="['entity-item', editorState.selectedEntityId === entity.id ? 'selected' : '']"
          @click="selectEntity(entity.id)"
        >
          <n-space align="center" :size="8">
            <n-icon size="16" :color="editorState.selectedEntityId === entity.id ? '#63e2b7' : '#a1a1aa'">
              <cube-outline />
            </n-icon>
            <n-text :strong="editorState.selectedEntityId === entity.id" 
                    :style="{ color: editorState.selectedEntityId === entity.id ? '#fff' : '#d4d4d8' }">
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
  background: #09090b; /* Zinc 950 */
}

.panel-header {
  padding: 10px 16px;
  background: #18181b; /* Zinc 900 */
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.1em;
  border-bottom: 1px solid #27272a;
}

.search-bar {
  padding: 12px;
  background: #09090b;
}

.empty-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #09090b;
}

.entity-list {
  flex: 1;
}

:deep(.n-list) {
  background: transparent;
}

:deep(.n-list-item) {
  padding: 6px 16px !important;
  background: transparent !important;
  transition: all 0.15s ease;
  border: none !important;
  cursor: pointer;
}

:deep(.n-list-item:hover) {
  background: #18181b !important;
}

.entity-item.selected {
  background: #1e1e2e !important; /* Slightly distinct blue-grey */
  position: relative;
}

.entity-item.selected::before {
  content: '';
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 3px;
  background: #63e2b7;
}
</style>
