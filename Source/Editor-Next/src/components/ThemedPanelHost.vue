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

  Sizing: NConfigProvider renders an outer <div>, NMessageProvider /
  NDialogProvider render Fragments. The ConfigProvider div sits between
  dockview's .dv-content-container (which uses `flex-grow:1; min-height:0`
  but no explicit height) and the panel root (which uses `height:100%`).
  Without `height:100%` on the ConfigProvider div the panel root falls
  back to intrinsic content height — its inner n-scrollbar then measures
  itself the same height as its content, sees scrollHeight==clientHeight,
  and never activates scroll. Result: when dockview gives the panel a
  smaller cell than its content needs (e.g. RenderTargetDebuggerPanel at
  the default 260px registered height collapsed by sibling panels), the
  buttons / form rows past the cell bottom get clipped by .dv-groupview's
  `overflow:hidden` and the user can't scroll to reveal them. Setting
  `height:100%` on the ConfigProvider wrapper restores the height chain
  so the panel is constrained to dockview's actual cell, the n-scrollbar
  measures correctly, and overflow becomes scrollable.
-->
<template>
  <n-config-provider
    :theme="activeTheme"
    :theme-overrides="themeOverrides"
    style="height: 100%"
  >
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
