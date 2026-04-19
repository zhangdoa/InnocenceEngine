<template>
  <n-config-provider :theme="activeTheme" :theme-overrides="themeOverrides" style="height: 100%">
    <n-message-provider>
      <n-dialog-provider>
        <app-layout />
      </n-dialog-provider>
    </n-message-provider>
  </n-config-provider>
</template>

<script setup>
import { NConfigProvider, NMessageProvider, NDialogProvider } from 'naive-ui'
import AppLayout from './components/AppLayout.vue'
import { useTheme } from './composables/useTheme'

// Initialize Catppuccin theme management
const { activeTheme, themeOverrides } = useTheme()
</script>

<style>
html, body, #app {
  height: 100%;
  margin: 0;
  padding: 0;
}

body {
  overflow: hidden;
  font-family: v-sans, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}

/* Force consistency on native inputs to prevent white background leaks */
input, select, textarea {
  background-color: var(--ctp-mantle) !important;
  color: var(--ctp-text) !important;
  border: 1px solid var(--ctp-surface1) !important;
}

/* Base Scrollbar */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}
::-webkit-scrollbar-track {
  background: var(--ctp-mantle);
}
::-webkit-scrollbar-thumb {
  background: var(--ctp-surface1);
  border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover {
  background: var(--ctp-surface2);
}
</style>
