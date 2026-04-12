<script setup>
import { 
  NCollapse, NCollapseItem, NText, NScrollbar, NEmpty, NH3, NIcon, NSpace
} from 'naive-ui'
import { CubeOutline, BulbOutline, SettingsOutline } from '@vicons/ionicons5'
import { sceneStore } from '../store/sceneStore'
import TransformEditor from './inspector/TransformEditor.vue'
import LightEditor from './inspector/LightEditor.vue'
</script>

<template>
  <div class="property-panel">
    <div v-if="!sceneStore.selectedEntity" class="no-selection">
      <n-empty description="No Selection" size="small" />
    </div>
    
    <n-scrollbar v-else class="properties-content">
      <div class="entity-info">
        <n-space align="center" :size="8">
          <n-icon size="20"><cube-outline /></n-icon>
          <n-h3 style="margin: 0;">{{ sceneStore.selectedEntity.name }}</n-h3>
        </n-space>
        <n-text depth="3" style="font-size: 10px; font-family: monospace; margin-left: 28px;">
          UID: {{ sceneStore.selectedEntity.id }}
        </n-text>
      </div>

      <n-collapse :default-expanded-names="['TransformComponent', 'LightComponent']" arrow-placement="right">
        <n-collapse-item 
          v-for="comp in sceneStore.selectedEntity.components" 
          :key="comp.type" 
          :name="comp.type"
        >
          <template #header>
            <n-space align="center" :size="8">
              <n-icon v-if="comp.type === 'TransformComponent'"><settings-outline /></n-icon>
              <n-icon v-else-if="comp.type === 'LightComponent'"><bulb-outline /></n-icon>
              <n-text strong>{{ comp.type }}</n-text>
            </n-space>
          </template>

          <!-- Component-specific Editors -->
          <transform-editor v-if="comp.type === 'TransformComponent'" :component="comp" />
          <light-editor v-else-if="comp.type === 'LightComponent'" :component="comp" />

          <div v-else>
            <n-text depth="3" style="font-size: 11px; font-style: italic;">No editable properties for this component.</n-text>
          </div>
        </n-collapse-item>
      </n-collapse>
    </n-scrollbar>
  </div>
</template>

<style scoped>
.property-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.no-selection {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.properties-content {
  flex: 1;
  padding: 24px 20px;
}

.entity-info {
  margin-bottom: 24px;
  padding-bottom: 16px;
  border-bottom: 1px solid #333;
}
</style>
