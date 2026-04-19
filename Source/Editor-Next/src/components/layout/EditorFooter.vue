<template>
  <footer class="editor-footer" data-test="editor-footer">
    <n-space justify="space-between" align="center" style="width: 100%; height: 100%; padding: 0 12px;">
      <n-space :size="8" align="center">
        <n-tag :type="statusTagType" size="tiny" round data-test="connection-status">
          {{ statusLabel }}
        </n-tag>
        <n-button
          v-if="connectionStore.status === 'giving-up'"
          size="tiny"
          type="primary"
          secondary
          data-test="connection-retry"
          @click="connectionStore.retryNow()"
        >
          Retry
        </n-button>
      </n-space>

      <n-text depth="3" class="flex-grow" style="font-size: 11px; font-family: monospace;">
        <n-icon style="vertical-align: middle; margin-right: 4px;"><terminal-outline /></n-icon>
        {{ connectionStore.lastMessage || 'System Idle' }}
      </n-text>

      <n-text depth="3" style="font-size: 10px;">v0.0.9</n-text>
    </n-space>
  </footer>
</template>

<script setup>
import { computed } from 'vue'
import { NSpace, NText, NIcon, NTag, NButton } from 'naive-ui'
import { TerminalOutline } from '@vicons/ionicons5'
import { connectionStore } from '../../store/connectionStore'

const STATUS_LABEL = {
  idle: 'Idle',
  connecting: 'Connecting',
  live: 'Live',
  lost: 'Lost',
  stopping: 'Stopping',
  'giving-up': 'Offline',
}

const STATUS_TAG = {
  idle: 'default',
  connecting: 'info',
  live: 'success',
  lost: 'warning',
  stopping: 'default',
  'giving-up': 'error',
}

const statusLabel = computed(() => STATUS_LABEL[connectionStore.status] ?? 'Unknown')
const statusTagType = computed(() => STATUS_TAG[connectionStore.status] ?? 'default')
</script>

<style scoped>
.editor-footer {
  height: 28px;
  flex-shrink: 0;
  background: var(--ctp-mantle);
  border-top: 1px solid var(--ctp-surface1);
}

.flex-grow {
  flex: 1;
  text-align: center;
}
</style>
