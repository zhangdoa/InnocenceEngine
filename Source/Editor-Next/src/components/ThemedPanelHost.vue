<!--
  Every dockview panel is mounted via dockview-vue's `mountVueComponent`,
  which copies `parent.appContext` but only merges *direct-parent*
  provides into the new VNode. NConfigProvider / NMessageProvider /
  NDialogProvider from App.vue are grandparents, so their provides don't
  reach the panel. Widgets inside panels end up using Naive UI's default
  theme (white inputs, green focus rings, etc).

  This host wraps the real panel in a fresh set of providers that match
  App.vue's, re-entering the theme chain at the panel boundary. Keep it
  reactive — `activeTheme` / `themeOverrides` are recomputed on flavor
  change by useTheme, and Naive re-renders when the props update.
-->
<template>
  <n-config-provider :theme="activeTheme" :theme-overrides="themeOverrides">
    <n-message-provider>
      <n-dialog-provider>
        <slot />
      </n-dialog-provider>
    </n-message-provider>
  </n-config-provider>
</template>

<script setup>
import { NConfigProvider, NMessageProvider, NDialogProvider } from 'naive-ui'
import { useTheme } from '../composables/useTheme'

const { activeTheme, themeOverrides } = useTheme()
</script>
