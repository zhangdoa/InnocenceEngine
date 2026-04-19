<template>
  <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
    <n-form-item label="Color">
      <n-color-picker 
        :value="rgbToHex(component.color)" 
        @update:value="updateColor"
        :modes="['hex']"
        :show-alpha="false"
      />
    </n-form-item>
    <n-form-item label="Luminous" style="margin-top: 12px;">
      <n-input-number 
        v-model:value="component.intensity" 
        @update:value="updateProp('intensity')" 
        :step="10"
      />
    </n-form-item>
  </n-form>
</template>

<script setup>
import { NForm, NFormItem, NInputNumber, NColorPicker } from 'naive-ui'
import { sceneStore } from '../../store/sceneStore'

const props = defineProps({
  component: {
    type: Object,
    required: true
  }
})

const updateProp = (property) => {
  sceneStore.updateProperty({
    id: sceneStore.selectedEntity.id,
    component: 'LightComponent',
    property,
    value: props.component[property]
  })
}

// #RRGGBB is the native <input type="color"> wire format, not a theme
// surface — leaving the literal so an unset color picker has a sensible
// default when the engine hasn't sent a component value yet.
const rgbToHex = (rgb) => {
  if (!rgb) return '#ffffff'
  const r = Math.round(rgb[0] * 255).toString(16).padStart(2, '0')
  const g = Math.round(rgb[1] * 255).toString(16).padStart(2, '0')
  const b = Math.round(rgb[2] * 255).toString(16).padStart(2, '0')
  return `#${r}${g}${b}`
}

const updateColor = (hex) => {
  const r = parseInt(hex.slice(1, 3), 16) / 255
  const g = parseInt(hex.slice(3, 5), 16) / 255
  const b = parseInt(hex.slice(5, 7), 16) / 255
  props.component.color = [r, g, b]
  updateProp('color')
}
</script>
