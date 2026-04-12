<template>
  <footer class="editor-footer">
    <n-space justify="space-between" align="center" style="width: 100%; height: 100%; padding: 0 12px;">
      <n-text depth="3" style="font-size: 10px; text-transform: uppercase; letter-spacing: 1px;">Ready</n-text>
      <n-text depth="3" class="flex-grow" style="font-size: 11px; font-family: monospace;">
        <n-icon style="vertical-align: middle; margin-right: 4px;"><terminal-outline /></n-icon>
        {{ connectionStore.lastMessage || 'System Idle' }}
      </n-text>
      <n-text depth="3" style="font-size: 10px;">v0.0.9</n-text>
    </n-space>
  </footer>
</template>

<script setup>
import { NSpace, NText, NIcon } from 'naive-ui'
import { TerminalOutline } from '@vicons/ionicons5'
import { connectionStore } from '../../store/connectionStore'
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
