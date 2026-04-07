<script setup>
import { ref, computed, defineProps, onMounted, watch } from 'vue'
import { NList, NListItem, NInput, NScrollbar, NText, NSpace } from 'naive-ui'
import { editorState } from '../store'

const props = defineProps({
  params: Object
})

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
  if (props.params?.onSelectEntity) {
    props.params.onSelectEntity(id)
  }
}
</script>

<template>
  <div class="hierarchy-panel">
    <div class="panel-header">
      <n-text depth="3" strong>Scene Hierarchy</n-text>
    </div>
    <div class="search-bar">
      <n-input size="small" placeholder="Search entities..." v-model:value="searchQuery" clearable>
        <template #prefix>
          <span>🔍</span>
        </template>
      </n-input>
    </div>
    <n-scrollbar class="entity-list">
      <n-list hoverable clickable size="small">
        <n-list-item 
          v-for="entity in filteredEntities" 
          :key="entity.id"
          :class="['entity-item', editorState.selectedEntityId === entity.id ? 'selected' : '']"
          @click="selectEntity(entity.id)"
        >
          <n-space align="center" :size="8">
            <n-text depth="3" style="font-size: 14px;">📦</n-text>
            <n-text style="font-size: 13px;">{{ entity.name }}</n-text>
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

.search-bar {
  padding: 8px;
  background: #18181c;
}

.entity-list {
  flex: 1;
}

.entity-item {
  padding: 4px 12px !important;
  transition: background 0.2s ease;
}

.entity-item.selected {
  background: #1a3a5a !important;
}

.entity-item.selected :deep(.n-text) {
  color: #fff !important;
}
</style>
