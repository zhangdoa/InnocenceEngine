<template>
  <n-config-provider :theme="darkTheme" :theme-overrides="themeOverrides" style="height: 100%">
    <n-message-provider>
      <app-layout />
    </n-message-provider>
  </n-config-provider>
</template>

<script setup>
import { NConfigProvider, NMessageProvider, darkTheme } from 'naive-ui'
import AppLayout from './components/AppLayout.vue'

/**
 * Catppuccin Macchiato Theme Overrides for Naive UI
 */
const themeOverrides = {
  common: {
    primaryColor: '#8aadf4', // Blue
    primaryColorHover: '#91d7e3', // Sky
    primaryColorPressed: '#7dc4e4', // Sapphire
    bodyColor: '#24273a', // Base
    cardColor: '#363a4f', // Surface0
    modalColor: '#1e2030', // Mantle
    popoverColor: '#1e2030', // Mantle
    textColorBase: '#cad3f5', // Text
    textColor1: '#cad3f5',
    textColor2: '#b8c0e0', // Subtext1
    textColor3: '#a5adcb', // Subtext0
    dividerColor: '#494d64', // Surface1
    borderColor: '#494d64',
  },
  Button: {
    textColorText: '#cad3f5',
  },
  Input: {
    color: '#1e2030',
    colorFocus: '#1e2030',
    border: '1px solid #494d64',
  },
  Menu: {
    itemColorActive: '#363a4f',
    itemTextColorActive: '#8aadf4',
    itemIconColorActive: '#8aadf4',
  },
  Tag: {
    colorSuccess: '#a6da95',
    colorError: '#ed8796',
    colorInfo: '#8aadf4',
    colorWarning: '#eed49f',
  }
}
</script>

<style>
/* Global Catppuccin Macchiato theme overrides for dockview */
.dockview-theme-abyssal {
  --dv-pane-background-color: #24273a;
  --dv-tabs-and-actions-container-background-color: #1e2030;
  --dv-activegroup-visiblepanel-tab-background-color: #24273a;
  --dv-inactivegroup-visiblepanel-tab-background-color: #363a4f;
  --dv-tab-text-color: #a5adcb;
  --dv-active-tab-text-color: #cad3f5;
  --dv-separator-color: #494d64;
  --dv-group-view-drop-target-color: rgba(138, 173, 244, 0.2);
}

html, body, #app {
  height: 100%;
  margin: 0;
  padding: 0;
  background: #1e2030; /* Mantle */
}

body {
  overflow: hidden;
  color: #cad3f5;
  font-family: v-sans, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}

/* Scrollbar styling for Catppuccin */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}
::-webkit-scrollbar-track {
  background: #1e2030;
}
::-webkit-scrollbar-thumb {
  background: #494d64;
  border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover {
  background: #5b6078;
}
</style>
