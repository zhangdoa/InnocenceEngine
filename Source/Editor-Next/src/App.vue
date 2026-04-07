<template>
  <n-config-provider :theme="activeTheme" :theme-overrides="themeOverrides" style="height: 100%">
    <n-message-provider>
      <app-layout />
    </n-message-provider>
  </n-config-provider>
</template>

<script setup>
import { computed, watch } from 'vue'
import { NConfigProvider, NMessageProvider, darkTheme, lightTheme } from 'naive-ui'
import AppLayout from './components/AppLayout.vue'
import { palette } from './palette'
import { editorState } from './store'

const activeTheme = computed(() => editorState.themeFlavor === 'latte' ? lightTheme : darkTheme)
const currentPalette = computed(() => palette[editorState.themeFlavor])

/**
 * Catppuccin Theme Overrides for Naive UI
 */
const themeOverrides = computed(() => ({
  common: {
    primaryColor: currentPalette.value.blue,
    primaryColorHover: currentPalette.value.sky,
    primaryColorPressed: currentPalette.value.sapphire,
    bodyColor: currentPalette.value.base,
    cardColor: currentPalette.value.surface0,
    modalColor: currentPalette.value.mantle,
    popoverColor: currentPalette.value.mantle,
    textColorBase: currentPalette.value.text,
    textColor1: currentPalette.value.text,
    textColor2: currentPalette.value.subtext1,
    textColor3: editorState.themeFlavor === 'latte' ? '#5c5f77' : currentPalette.value.subtext0,
    dividerColor: currentPalette.value.surface1,
    borderColor: currentPalette.value.surface1,
  },
  Form: {
    labelTextColor: currentPalette.value.text,
  },
  Input: {
    color: currentPalette.value.mantle,
    colorFocus: currentPalette.value.mantle,
    textColor: currentPalette.value.text,
    border: `1px solid ${currentPalette.value.surface1}`,
    placeholderColor: currentPalette.value.overlay0,
  },
  InputNumber: {
    color: currentPalette.value.mantle,
    textColor: currentPalette.value.text,
  },
  Menu: {
    itemColorActive: currentPalette.value.surface0,
    itemTextColorActive: currentPalette.value.blue,
    itemIconColorActive: currentPalette.value.blue,
    itemTextColor: currentPalette.value.text,
    itemIconColor: currentPalette.value.text,
  },
  Tag: {
    colorSuccess: currentPalette.value.green,
    colorError: currentPalette.value.red,
    colorInfo: currentPalette.value.blue,
    colorWarning: currentPalette.value.yellow,
    textColorSuccess: '#11111b',
    textColorError: '#11111b',
    textColorInfo: '#11111b',
    textColorWarning: '#11111b',
  },
  Collapse: {
    titleTextColor: currentPalette.value.text,
    contentTextColor: currentPalette.value.text,
  },
  List: {
    color: 'transparent',
    textColor: currentPalette.value.text,
  }
}))

// Update global CSS variables dynamically when theme changes
watch(() => editorState.themeFlavor, (newFlavor) => {
  const p = palette[newFlavor];
  const root = document.documentElement;
  Object.keys(p).forEach(key => {
    root.style.setProperty(`--ctp-${key}`, p[key]);
  });
}, { immediate: true });
</script>

<style>
/* Global Catppuccin theme overrides for dockview */
.dockview-theme-abyssal {
  --dv-pane-background-color: var(--ctp-base);
  --dv-tabs-and-actions-container-background-color: var(--ctp-mantle);
  --dv-activegroup-visiblepanel-tab-background-color: var(--ctp-base);
  --dv-inactivegroup-visiblepanel-tab-background-color: var(--ctp-surface0);
  --dv-tab-text-color: var(--ctp-subtext0);
  --dv-active-tab-text-color: var(--ctp-text);
  --dv-separator-color: var(--ctp-surface1);
  --dv-group-view-drop-target-color: rgba(138, 173, 244, 0.2);
}

html, body, #app {
  height: 100%;
  margin: 0;
  padding: 0;
  background: var(--ctp-mantle);
  transition: background 0.3s ease;
}

body {
  overflow: hidden;
  color: var(--ctp-text);
  font-family: v-sans, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}

/* Force consistency on all native inputs to prevent white background leaks */
input, select, textarea {
  background-color: var(--ctp-mantle) !important;
  color: var(--ctp-text) !important;
  border: 1px solid var(--ctp-surface1) !important;
}

/* 
 * CRITICAL UX FIX: Target Naive UI internal elements with extreme precision.
 * These selectors are based on the diagnostic audit failures.
 */
.n-form-item-label .n-form-item-label__text {
  color: var(--ctp-text) !important;
}

.n-tag .n-tag__content {
  color: #11111b !important; /* Always dark text on bright tags for contrast */
}

/* Fix for menu headers which were failing in some dark themes */
.n-menu-item-content-header {
  color: var(--ctp-text) !important;
}

/* Scrollbar styling for Catppuccin */
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
