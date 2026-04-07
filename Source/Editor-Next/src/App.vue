<template>
  <n-config-provider :theme="darkTheme" style="height: 100%">
    <n-message-provider>
      <app-layout />
    </n-message-provider>
  </n-config-provider>
</template>

<script setup>
import { NConfigProvider, NMessageProvider, darkTheme } from 'naive-ui'
import AppLayout from './components/AppLayout.vue'
</script>

<style>
/* Global abyssal theme overrides for dockview */
.dockview-theme-abyssal {
  --dv-pane-background-color: #1e1e1e;
  --dv-tabs-and-actions-container-background-color: #252526;
  --dv-activegroup-visiblepanel-tab-background-color: #1e1e1e;
  --dv-inactivegroup-visiblepanel-tab-background-color: #2d2d2d;
  --dv-tab-text-color: #aaa;
  --dv-active-tab-text-color: #fff;
  --dv-separator-color: #333;
}

html, body, #app {
  height: 100%;
  margin: 0;
  padding: 0;
}

body {
  overflow: hidden;
  background: #18181c;
  color: #ccc;
  font-family: v-sans, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif, "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol";
}
</style>
