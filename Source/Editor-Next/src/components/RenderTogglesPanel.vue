<script setup>
import { onMounted, watch } from 'vue'
import {
  NScrollbar,
  NEmpty,
  NSpace,
  NSwitch,
  NText,
  NButton,
  NDivider,
  useMessage,
} from 'naive-ui'
import { devToggleStore } from '../store/devToggleStore'
import { connectionStore } from '../store/connectionStore'

const message = useMessage()

// Pull the current list once we're connected. Re-pull on reconnect so a
// new engine session populates the panel without a manual refresh.
watch(
  () => connectionStore.isConnected,
  (connected) => {
    if (connected) devToggleStore.refresh()
    else devToggleStore.reset()
  },
  { immediate: false },
)

onMounted(() => {
  if (connectionStore.isConnected) devToggleStore.refresh()
})

const onToggleChange = (toggle, value) => {
  devToggleStore.setToggle(toggle.name, value)
}

const onActionTrigger = (action) => {
  devToggleStore.triggerAction(action.name)
  message.success(`${action.name} triggered`)
}
</script>

<template>
  <div class="render-toggles" data-test="render-toggles-panel">
    <div v-if="!connectionStore.isConnected" class="empty-container">
      <n-empty description="Engine offline" size="small" />
    </div>

    <n-scrollbar v-else>
      <div class="section">
        <n-text depth="3" class="section-label">Toggles</n-text>
        <n-empty
          v-if="!devToggleStore.toggles.length"
          description="No toggles registered"
          size="small"
        />
        <div
          v-for="toggle in devToggleStore.toggles"
          :key="toggle.name"
          class="row"
          :data-test="`toggle-row-${toggle.name}`"
        >
          <n-text>{{ toggle.name }}</n-text>
          <n-switch
            :value="toggle.value"
            @update:value="(v) => onToggleChange(toggle, v)"
            :data-test="`toggle-switch-${toggle.name}`"
          />
        </div>
      </div>

      <n-divider style="margin: 8px 0" />

      <div class="section">
        <n-text depth="3" class="section-label">Actions</n-text>
        <n-empty
          v-if="!devToggleStore.actions.length"
          description="No actions registered"
          size="small"
        />
        <n-space :size="8" class="action-row">
          <n-button
            v-for="action in devToggleStore.actions"
            :key="action.name"
            size="small"
            :data-test="`action-btn-${action.name}`"
            @click="onActionTrigger(action)"
          >
            {{ action.name }}
          </n-button>
        </n-space>
      </div>
    </n-scrollbar>
  </div>
</template>

<style scoped>
.render-toggles {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.empty-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.section {
  padding: 8px 12px;
}

.section-label {
  display: block;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 6px;
}

.row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 0;
}

.action-row {
  flex-wrap: wrap;
}
</style>
