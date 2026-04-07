<script setup>
import { 
  NCollapse, NCollapseItem, NForm, NFormItem, NGrid, NGridItem, 
  NInputNumber, NColorPicker, NText, NScrollbar, NEmpty, NH3, NIcon, NSpace
} from 'naive-ui'
import { CubeOutline, BulbOutline, SettingsOutline } from '@vicons/ionicons5'
import { editorState } from '../store'

const updateProp = (component, property, value) => {
  editorState.updateProperty({
    id: editorState.selectedEntity.id,
    component,
    property,
    value
  })
}

const rgbToHex = (rgb) => {
  if (!rgb) return '#FFFFFF'
  const r = Math.round(rgb[0] * 255).toString(16).padStart(2, '0')
  const g = Math.round(rgb[1] * 255).toString(16).padStart(2, '0')
  const b = Math.round(rgb[2] * 255).toString(16).padStart(2, '0')
  return `#${r}${g}${b}`
}

const updateColor = (comp, hex) => {
  const r = parseInt(hex.slice(1, 3), 16) / 255
  const g = parseInt(hex.slice(3, 5), 16) / 255
  const b = parseInt(hex.slice(5, 7), 16) / 255
  comp.color = [r, g, b]
  updateProp(comp.type, 'color', comp.color)
}
</script>

<template>
  <div class="property-panel">
    <div v-if="!editorState.selectedEntity" class="no-selection">
      <n-empty description="No Selection" size="small" />
    </div>
    
    <n-scrollbar v-else class="properties-content">
      <div class="entity-info">
        <n-space align="center" :size="8">
          <n-icon size="20" color="#8aadf4"><cube-outline /></n-icon>
          <n-h3 style="margin: 0; color: #cad3f5;">{{ editorState.selectedEntity.name }}</n-h3>
        </n-space>
        <n-text depth="3" style="font-size: 10px; font-family: monospace; margin-left: 28px;">
          UID: {{ editorState.selectedEntity.id }}
        </n-text>
      </div>

      <n-collapse :default-expanded-names="['TransformComponent', 'LightComponent']" arrow-placement="right">
        <n-collapse-item 
          v-for="comp in editorState.selectedEntity.components" 
          :key="comp.type" 
          :name="comp.type"
        >
          <template #header>
            <n-space align="center" :size="8">
              <n-icon v-if="comp.type === 'TransformComponent'" color="#91d7e3"><settings-outline /></n-icon>
              <n-icon v-else-if="comp.type === 'LightComponent'" color="#eed49f"><bulb-outline /></n-icon>
              <n-text strong style="color: #cad3f5;">{{ comp.type }}</n-text>
            </n-space>
          </template>

          <!-- Transform Component -->
          <template v-if="comp.type === 'TransformComponent'">
            <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
              <n-form-item label="Position">
                <n-grid :cols="3" :x-gap="6">
                  <n-grid-item>
                    <n-input-number v-model:value="comp.pos[0]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px; color: #ed8796;">X</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                  <n-grid-item>
                    <n-input-number v-model:value="comp.pos[1]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px; color: #a6da95;">Y</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                  <n-grid-item>
                    <n-input-number v-model:value="comp.pos[2]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px; color: #8aadf4;">Z</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                </n-grid>
              </n-form-item>
              
              <n-form-item label="Rotation" style="margin-top: 12px;">
                <n-text depth="3" style="font-size: 10px; font-family: monospace;">
                  {{ comp.rot.map(v => v.toFixed(3)).join(', ') }}
                </n-text>
              </n-form-item>

              <n-form-item label="Scale" style="margin-top: 12px;">
                <n-grid :cols="3" :x-gap="6">
                  <n-grid-item>
                    <n-input-number v-model:value="comp.scale[0]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px;">X</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                  <n-grid-item>
                    <n-input-number v-model:value="comp.scale[1]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px;">Y</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                  <n-grid-item>
                    <n-input-number v-model:value="comp.scale[2]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false">
                      <template #prefix><n-text depth="3" style="font-size: 10px;">Z</n-text></template>
                    </n-input-number>
                  </n-grid-item>
                </n-grid>
              </n-form-item>
            </n-form>
          </template>

          <!-- Light Component -->
          <template v-else-if="comp.type === 'LightComponent'">
            <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
              <n-form-item label="Color">
                <n-color-picker 
                  :value="rgbToHex(comp.color)" 
                  @update:value="(hex) => updateColor(comp, hex)"
                  :modes="['hex']"
                  :show-alpha="false"
                />
              </n-form-item>
              <n-form-item label="Luminous" style="margin-top: 12px;">
                <n-input-number 
                  v-model:value="comp.intensity" 
                  @update:value="updateProp(comp.type, 'intensity', comp.intensity)" 
                  :step="10"
                />
              </n-form-item>
            </n-form>
          </template>

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
  background: #24273a; /* Catppuccin Base */
}

.no-selection {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #24273a;
}

.properties-content {
  flex: 1;
  padding: 24px 20px; /* Increased padding to avoid sticking to edges */
}

.entity-info {
  margin-bottom: 24px;
  padding-bottom: 16px;
  border-bottom: 1px solid #494d64;
}

:deep(.n-collapse-item) {
  margin-bottom: 16px;
  background: #363a4f; /* Catppuccin Surface0 */
  padding: 8px 16px;
  border-radius: 8px;
  border: 1px solid #494d64;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
}

:deep(.n-collapse-item__header) {
  padding: 10px 0 !important;
}

:deep(.n-collapse-item__content-inner) {
  padding: 16px 0 12px 0 !important;
}

:deep(.n-input-number) {
  background: #24273a;
}

:deep(.n-form-item-label) {
  color: #a5adcb !important; /* Catppuccin Subtext0 */
  font-size: 12px;
}
</style>
