<template>
  <header class="editor-header">
    <div class="header-left">
      <n-menu mode="horizontal" :options="menuOptions" class="menu-bar" @update:value="handleMenuClick" />
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
import { h } from 'vue'
import { 
  NMenu, NButton, NButtonGroup, NTag, NSpace, NIcon, useMessage 
} from 'naive-ui'
import { 
  PowerOutline, RefreshOutline, SaveOutline, 
  CheckmarkCircle, CloseCircle, SettingsOutline,
  ColorPaletteOutline, SunnyOutline, FlaskOutline, MoonOutline
} from '@vicons/ionicons5'
import { connectionStore } from '../../store/connectionStore'
import { sceneStore } from '../../store/sceneStore'
import { uiStore } from '../../store/uiStore'

const message = useMessage()

const renderIcon = (icon) => {
  return () => h(NIcon, null, { default: () => h(icon) })
}

const menuOptions = [
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
  }
]

const handleMenuClick = (key) => {
  if (key.startsWith('theme-')) {
    const flavor = key.replace('theme-', '');
    uiStore.setTheme(flavor);
    message.info(`Theme changed to ${flavor}`);
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
