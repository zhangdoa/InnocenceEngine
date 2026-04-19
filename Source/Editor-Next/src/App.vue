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

/* Native controls outside of Naive UI get Catppuccin surfaces (prevents white
 * background leaks on raw <input>). Naive UI's internal <input> carries
 * .n-input__input-el and is themed via themeOverrides in useTheme.js — skip
 * it here so we don't double-style and get mantle-colored corners inside a
 * Naive surface. */
input:not(.n-input__input-el):not(.n-input-number-input),
select:not(.n-base-selection-input),
textarea:not(.n-input__textarea-el) {
  background-color: var(--ctp-mantle);
  color: var(--ctp-text);
  border: 1px solid var(--ctp-surface1);
}

/* Catppuccin dockview theme — maps every --dv-* var to --ctp-* so tabs,
 * group backgrounds, sashes, and drag targets follow the active flavor.
 * Upstream .dockview-theme-dark hardcodes VS Code greys and would not repaint
 * on flavor change. */
.dockview-theme-ctp {
  --dv-paneview-active-outline-color: var(--ctp-blue);
  --dv-tabs-and-actions-container-font-size: 13px;
  --dv-tabs-and-actions-container-height: 32px;
  --dv-drag-over-background-color: color-mix(in srgb, var(--ctp-blue) 35%, transparent);
  --dv-drag-over-border-color: var(--ctp-blue);
  --dv-tabs-container-scrollbar-color: var(--ctp-overlay0);
  --dv-icon-hover-background-color: var(--ctp-surface1);
  --dv-floating-box-shadow: 0 4px 12px 0 color-mix(in srgb, var(--ctp-crust) 60%, transparent);
  --dv-overlay-z-index: 999;
  --dv-tab-font-size: inherit;
  --dv-border-radius: 0px;
  --dv-tab-margin: 0;
  --dv-sash-color: transparent;
  --dv-active-sash-color: var(--ctp-blue);
  --dv-active-sash-transition-duration: 0.1s;
  --dv-active-sash-transition-delay: 0.5s;
  --dv-group-view-background-color: var(--ctp-base);
  --dv-tabs-and-actions-container-background-color: var(--ctp-mantle);
  --dv-activegroup-visiblepanel-tab-background-color: var(--ctp-base);
  --dv-activegroup-hiddenpanel-tab-background-color: var(--ctp-mantle);
  --dv-inactivegroup-visiblepanel-tab-background-color: var(--ctp-base);
  --dv-inactivegroup-hiddenpanel-tab-background-color: var(--ctp-mantle);
  --dv-tab-divider-color: var(--ctp-surface0);
  --dv-activegroup-visiblepanel-tab-color: var(--ctp-text);
  --dv-activegroup-hiddenpanel-tab-color: var(--ctp-subtext0);
  --dv-inactivegroup-visiblepanel-tab-color: var(--ctp-subtext1);
  --dv-inactivegroup-hiddenpanel-tab-color: var(--ctp-overlay0);
  --dv-separator-border: var(--ctp-surface0);
  --dv-paneview-header-border-color: var(--ctp-surface1);
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
