<script setup>
import { ref, computed } from 'vue'
import {
  NList, NListItem, NInput, NScrollbar, NText, NSpace, NIcon,
  NEmpty, NButton, NButtonGroup, useDialog,
} from 'naive-ui'
import {
  CubeOutline, SearchOutline, AddOutline, TrashOutline, CreateOutline,
} from '@vicons/ionicons5'
import { sceneStore } from '../store/sceneStore'
import { connectionStore } from '../store/connectionStore'

const dialog = useDialog()

const searchQuery = ref('')

// Split the engine's flat entity name into a (qualifier, label) pair so the
// outliner can disambiguate look-alikes such as `Bunny` (top-level placeholder
// in GISponza) versus `GISponza.Bunny.0` (mesh instance loaded from the
// GISponza.Bunny sub-scene). Both are real distinct entities; without this
// split they read as duplicates in the search results. Splitting on the first
// dot matches the engine's sub-scene-prefix convention (sub-scene entities
// inherit the parent scene's name as a leading qualifier).
const splitEntityName = (name) => {
  if (!name) return { qualifier: '', label: '' }
  const dot = name.indexOf('.')
  if (dot === -1) return { qualifier: '', label: name }
  return { qualifier: name.slice(0, dot), label: name.slice(dot + 1) }
}

const filteredEntities = computed(() => {
  const entities = sceneStore.entities || []
  const decorated = entities.map(e => ({ ...e, ...splitEntityName(e.name) }))
  if (!searchQuery.value) return decorated
  const q = searchQuery.value.toLowerCase()
  return decorated.filter(e => e.name.toLowerCase().includes(q))
})

const selectEntity = (id) => {
  sceneStore.selectEntity(id)
}

const onCreate = () => {
  dialog.create({
    title: 'New entity',
    content: () => 'Name for the new entity:',
    positiveText: 'Create',
    negativeText: 'Cancel',
    showInput: true,
    inputProps: { placeholder: 'Entity', defaultValue: 'NewEntity' },
    onPositiveClick: ({ inputValue }) => {
      const name = (inputValue || '').trim() || 'NewEntity'
      sceneStore.createEntity(name)
    },
  })
}

const onRename = () => {
  const id = sceneStore.selectedEntityId
  if (!id) return
  const current = sceneStore.entities.find((e) => e.id === id)
  dialog.create({
    title: 'Rename entity',
    content: () => `Rename "${current?.name ?? ''}" to:`,
    positiveText: 'Rename',
    negativeText: 'Cancel',
    showInput: true,
    inputProps: { defaultValue: current?.name ?? '' },
    onPositiveClick: ({ inputValue }) => {
      const name = (inputValue || '').trim()
      if (!name) return
      sceneStore.renameEntity(id, name)
    },
  })
}

const onDelete = () => {
  const id = sceneStore.selectedEntityId
  if (!id) return
  const current = sceneStore.entities.find((e) => e.id === id)
  dialog.warning({
    title: 'Delete entity',
    content: `Delete "${current?.name ?? ''}"? This cannot be undone.`,
    positiveText: 'Delete',
    negativeText: 'Cancel',
    onPositiveClick: () => sceneStore.deleteEntity(id),
  })
}
</script>

<template>
  <div class="hierarchy-panel">
    <div class="search-bar">
      <n-space align="center" :size="6" :wrap="false">
        <n-input size="small" placeholder="Search outliner..." v-model:value="searchQuery" clearable>
          <template #prefix>
            <n-icon><search-outline /></n-icon>
          </template>
        </n-input>
        <n-button-group size="small">
          <n-button
            quaternary
            :disabled="!connectionStore.isConnected"
            data-test="entity-create"
            title="New entity"
            @click="onCreate"
          >
            <template #icon><n-icon><add-outline /></n-icon></template>
          </n-button>
          <n-button
            quaternary
            :disabled="!sceneStore.selectedEntityId"
            data-test="entity-rename"
            title="Rename selected"
            @click="onRename"
          >
            <template #icon><n-icon><create-outline /></n-icon></template>
          </n-button>
          <n-button
            quaternary
            :disabled="!sceneStore.selectedEntityId"
            data-test="entity-delete"
            title="Delete selected"
            @click="onDelete"
          >
            <template #icon><n-icon><trash-outline /></n-icon></template>
          </n-button>
        </n-button-group>
      </n-space>
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
          <n-space align="center" :size="8" :wrap="false">
            <n-icon size="16">
              <cube-outline />
            </n-icon>
            <span class="entity-label">
              <n-text :strong="sceneStore.selectedEntityId === entity.id">
                {{ entity.label }}
              </n-text>
              <n-text v-if="entity.qualifier" depth="3" class="entity-qualifier">
                {{ entity.qualifier }}
              </n-text>
            </span>
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

.entity-label {
  display: inline-flex;
  align-items: baseline;
  gap: 8px;
  min-width: 0;
}

.entity-qualifier {
  font-size: 11px;
}
</style>
