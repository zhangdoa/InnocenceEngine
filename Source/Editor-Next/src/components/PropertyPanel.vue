<template>
  <div class="property-panel">
    <div class="panel-header">
      <n-text depth="3" strong>Properties</n-text>
    </div>
    
    <div v-if="!props.params?.selectedEntity" class="no-selection">
      <n-empty description="Select an entity to view properties" />
    </div>
    
    <n-scrollbar v-else class="properties-content">
      <div class="entity-info">
        <n-h3 style="margin: 0;">{{ props.params.selectedEntity.name }}</n-h3>
        <n-text depth="3" style="font-size: 10px;">ID: {{ props.params.selectedEntity.id }}</n-text>
      </div>

      <n-collapse :default-expanded-names="['TransformComponent', 'LightComponent']">
        <n-collapse-item 
          v-for="comp in props.params.selectedEntity.components" 
          :key="comp.type" 
          :title="comp.type" 
          :name="comp.type"
        >
          <!-- Transform Component -->
          <template v-if="comp.type === 'TransformComponent'">
            <n-form label-placement="left" label-width="60" size="small">
              <n-form-item label="Position">
                <n-grid :cols="3" :x-gap="4">
                  <n-grid-item><n-input-number v-model:value="comp.pos[0]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false" placeholder="X"><template #prefix><n-text depth="3">X</n-text></template></n-input-number></n-grid-item>
                  <n-grid-item><n-input-number v-model:value="comp.pos[1]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false" placeholder="Y"><template #prefix><n-text depth="3">Y</n-text></template></n-input-number></n-grid-item>
                  <n-grid-item><n-input-number v-model:value="comp.pos[2]" @update:value="updateProp(comp.type, 'pos', comp.pos)" :show-button="false" placeholder="Z"><template #prefix><n-text depth="3">Z</n-text></template></n-input-number></n-grid-item>
                </n-grid>
              </n-form-item>
              <n-form-item label="Scale">
                <n-grid :cols="3" :x-gap="4">
                  <n-grid-item><n-input-number v-model:value="comp.scale[0]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false" placeholder="X"><template #prefix><n-text depth="3">X</n-text></template></n-input-number></n-grid-item>
                  <n-grid-item><n-input-number v-model:value="comp.scale[1]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false" placeholder="Y"><template #prefix><n-text depth="3">Y</n-text></template></n-input-number></n-grid-item>
                  <n-grid-item><n-input-number v-model:value="comp.scale[2]" @update:value="updateProp(comp.type, 'scale', comp.scale)" :show-button="false" placeholder="Z"><template #prefix><n-text depth="3">Z</n-text></template></n-input-number></n-grid-item>
                </n-grid>
              </n-form-item>
            </n-form>
          </template>

          <!-- Light Component -->
          <template v-else-if="comp.type === 'LightComponent'">
            <n-form label-placement="left" label-width="60" size="small">
              <n-form-item label="Color">
                <n-color-picker 
                  :value="rgbToHex(comp.color)" 
                  @update:value="(hex) => updateColor(comp, hex)"
                  :modes="['hex']"
                />
              </n-form-item>
              <n-form-item label="Intensity">
                <n-input-number 
                  v-model:value="comp.intensity" 
                  @update:value="updateProp(comp.type, 'intensity', comp.intensity)" 
                  :step="10"
                />
              </n-form-item>
            </n-form>
          </template>

          <div v-else>
            <n-text depth="3" style="font-size: 11px;">Component fields not yet implemented</n-text>
          </div>
        </n-collapse-item>
      </n-collapse>
    </n-scrollbar>
  </div>
</template>

<script setup>
import { defineProps } from 'vue'
import { 
  NCollapse, NCollapseItem, NForm, NFormItem, NGrid, NGridItem, 
  NInputNumber, NColorPicker, NText, NScrollbar, NEmpty, NH3 
} from 'naive-ui'

const props = defineProps({
  params: Object
})

const updateProp = (component, property, value) => {
  if (props.params?.onUpdateProperty && props.params?.selectedEntity) {
    props.params.onUpdateProperty({
      id: props.params.selectedEntity.id,
      component,
      property,
      value
    })
  }
}

const rgbToHex = (rgb) => {
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

<style scoped>
.property-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #18181c;
}

.panel-header {
  padding: 8px 12px;
  background: #262629;
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid #333;
}

.no-selection {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.properties-content {
  flex: 1;
  padding: 12px;
}

.entity-info {
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 1px solid #333;
}

:deep(.n-collapse-item) {
  margin-bottom: 4px;
}

:deep(.n-collapse-item__content-inner) {
  padding-top: 12px !important;
}

:deep(.n-form-item) {
  margin-bottom: 8px;
}
</style>
