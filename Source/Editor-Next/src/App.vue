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

import './theme/index.css'

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
  background-color: var(--ctp-base);
  color: var(--ctp-text);
}
</style>
