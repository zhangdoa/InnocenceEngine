<template>
  <header class="editor-header">
    <div class="header-left">
      <n-menu mode="horizontal" :options="menuOptions" :value="null" class="menu-bar" @update:value="handleMenuClick" />
    </div>
    <div class="header-right">
      <n-space align="center" :size="20">
        <n-button-group size="small">
          <n-button @click="sceneStore.saveScene()" secondary title="Save current scene">
            <template #icon><n-icon><save-outline /></n-icon></template>
            Save
          </n-button>
          <n-button @click="connectionStore.restartEngine()" secondary title="Restart Engine sidecar">
            <template #icon><n-icon><refresh-outline /></n-icon></template>
            Restart
          </n-button>
          <n-button @click="connectionStore.stopEngine()" type="error" ghost title="Stop Engine sidecar">
            <template #icon><n-icon><power-outline /></n-icon></template>
            Stop
          </n-button>
        </n-button-group>
        <n-tag :type="connectionStore.isConnected ? 'success' : 'error'" size="small" round ghost>
          <template #icon>
            <n-icon>
              <checkmark-circle v-if="connectionStore.isConnected" />
              <close-circle v-else />
            </n-icon>
          </template>
          {{ connectionStore.isConnected ? 'Live' : 'Offline' }}
        </n-tag>
      </n-space>
    </div>
  </header>
</template>

<script setup>
import { h, computed } from 'vue'
import {
  NMenu, NButton, NButtonGroup, NTag, NSpace, NIcon, useMessage
} from 'naive-ui'
import {
  PowerOutline, RefreshOutline, SaveOutline,
  CheckmarkCircle, CloseCircle, SettingsOutline,
  ColorPaletteOutline, SunnyOutline, FlaskOutline, MoonOutline,
  AppsOutline, SquareOutline, CheckboxOutline
} from '@vicons/ionicons5'
import { connectionStore } from '../../store/connectionStore'
import { sceneStore } from '../../store/sceneStore'
import { uiStore } from '../../store/uiStore'
import { panelStore } from '../../store/panelStore'

const message = useMessage()

const renderIcon = (icon) => {
  return () => h(NIcon, null, { default: () => h(icon) })
}

// Built lazily so visibility-marker icons (filled vs outlined checkbox)
// re-render when panelStore.panels changes.
const menuOptions = computed(() => [
  {
    label: 'Editor',
    key: 'editor',
    icon: renderIcon(SettingsOutline),
    children: [
      {
        label: 'Theme',
        key: 'theme',
        icon: renderIcon(ColorPaletteOutline),
        children: [
          { label: 'Latte (Light)', key: 'theme-latte', icon: renderIcon(SunnyOutline) },
          { label: 'Frappé', key: 'theme-frappe', icon: renderIcon(FlaskOutline) },
          { label: 'Macchiato', key: 'theme-macchiato', icon: renderIcon(MoonOutline) },
          { label: 'Mocha (Dark)', key: 'theme-mocha', icon: renderIcon(MoonOutline) }
        ]
      }
    ]
  },
  {
    label: 'Window',
    key: 'window',
    icon: renderIcon(AppsOutline),
    children: [
      ...panelStore.panels.map((p) => ({
        label: p.title,
        key: `panel-toggle-${p.id}`,
        icon: renderIcon(p.visible ? CheckboxOutline : SquareOutline),
        props: { 'data-test': `window-toggle-${p.id}` },
      })),
      { type: 'divider', key: 'window-divider' },
      {
        label: 'Reset layout',
        key: 'window-reset',
        icon: renderIcon(RefreshOutline),
        props: { 'data-test': 'window-reset' },
      },
    ],
  },
])

const handleMenuClick = (key) => {
  if (key.startsWith('theme-')) {
    const flavor = key.replace('theme-', '');
    uiStore.setTheme(flavor);
    message.info(`Theme changed to ${flavor}`);
  } else if (key.startsWith('panel-toggle-')) {
    const id = key.replace('panel-toggle-', '');
    panelStore.toggle(id);
  } else if (key === 'window-reset') {
    panelStore.resetLayout();
    message.info('Panel layout reset');
  }
}
</script>

<style scoped>
.editor-header {
  height: 52px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20px;
  background: var(--ctp-mantle);
  border-bottom: 1px solid var(--ctp-surface1);
  user-select: none;
}

.header-left { 
  display: flex; 
  align-items: center; 
  gap: 32px;
  flex: 1;
}

.menu-bar { 
  flex: 1;
  min-width: 300px;
  border: none !important; 
  background: transparent !important;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 20px;
  flex-shrink: 0;
  min-width: 400px;
  justify-content: flex-end;
}
</style>
