<template>
  <div class="hierarchy-panel">
    <div class="panel-header">Scene Hierarchy</div>
    <div class="search-bar">
      <input type="text" placeholder="Search entities..." v-model="searchQuery" />
    </div>
    <div class="entity-list">
      <div 
        v-for="entity in filteredEntities" 
        :key="entity.id"
        class="entity-item"
        :class="{ selected: selectedEntityId === entity.id }"
        @click="selectEntity(entity.id)"
      >
        <span class="icon">📦</span>
        <span class="name">{{ entity.name }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, defineProps, defineEmits } from 'vue'

const props = defineProps({
  entities: Array,
  selectedEntityId: Number,
  onSelectEntity: Function
})

const searchQuery = ref('')

const filteredEntities = computed(() => {
  if (!searchQuery.value) return props.entities
  return props.entities.filter(e => 
    e.name.toLowerCase().includes(searchQuery.value.toLowerCase())
  )
})

const selectEntity = (id) => {
  if (props.onSelectEntity) {
    props.onSelectEntity(id)
  }
}
</script>

<style scoped>
.hierarchy-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #252526;
  color: #ccc;
  font-family: sans-serif;
}

.panel-header {
  padding: 8px 12px;
  background: #2d2d2d;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid #333;
}

.search-bar {
  padding: 8px;
}

.search-bar input {
  width: 100%;
  background: #3c3c3c;
  border: 1px solid #555;
  color: #fff;
  padding: 4px 8px;
  font-size: 12px;
  outline: none;
}

.entity-list {
  flex: 1;
  overflow-y: auto;
}

.entity-item {
  padding: 4px 12px;
  font-size: 13px;
  display: flex;
  align-items: center;
  cursor: pointer;
  user-select: none;
}

.entity-item:hover {
  background: #2a2d2e;
}

.entity-item.selected {
  background: #094771;
  color: #fff;
}

.entity-item .icon {
  margin-right: 8px;
  font-size: 12px;
  opacity: 0.7;
}
</style>
