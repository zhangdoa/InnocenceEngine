<script setup>
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
import { useIpc } from '../composables/useIpc'

const message = useMessage()
const ipc = useIpc()

// devToggleStore self-refreshes on connect and clears on disconnect via its
// own on('engine-connected') subscription.

// Screenshot is the first dev-action with a deferred result event
// (SCREENSHOT_SAVED, broadcast by EditorService after the rendering client
// finishes the save). The toast names the absolute saved path on success,
// or the failure reason on error — replacing the optimistic
// "Screenshot triggered" feedback that gave no actionable information.
ipc.on('SCREENSHOT_SAVED', (payload) => {
  if (payload && payload.ok) {
    message.success(`Screenshot saved: ${payload.path}`)
  } else {
    const reason = (payload && payload.error) || 'unknown error'
    message.error(`Screenshot failed: ${reason}`)
  }
})

const onToggleChange = (toggle, value) => {
  devToggleStore.setToggle(toggle.name, value)
}

const onActionTrigger = (action) => {
  devToggleStore.triggerAction(action.name)
  // Screenshot's result toast is driven by the SCREENSHOT_SAVED event
  // above; other actions keep the optimistic-toast path until they grow
  // their own per-action result events.
  if (action.name !== 'Screenshot') {
    message.success(`${action.name} triggered`)
  }
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
