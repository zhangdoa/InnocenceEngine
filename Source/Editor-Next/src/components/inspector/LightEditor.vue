<template>
  <n-form label-placement="left" label-width="105" size="small" :show-feedback="false">
    <n-form-item label="Color">
      <n-color-picker
        :value="rgbArrayToHex(draft.color)"
        @update:value="onColor"
        :modes="['hex']"
        :show-alpha="false"
      />
    </n-form-item>
    <n-form-item label="Use Temp." style="margin-top: 12px;">
      <n-checkbox
        :checked="draft.useColorTemperature"
        @update:checked="onUseColorTemperature"
      />
    </n-form-item>
    <n-form-item label="Temperature" style="margin-top: 12px;">
      <n-input-number
        :value="draft.colorTemperature"
        @update:value="onColorTemperature"
        :disabled="!draft.useColorTemperature"
        :step="100"
        :min="1000"
        :max="12000"
      />
    </n-form-item>
    <n-form-item label="Luminous" style="margin-top: 12px;">
      <n-input-number
        :value="draft.intensity"
        @update:value="onIntensity"
        :step="10"
      />
    </n-form-item>
    <n-form-item label="Cast Shadow" style="margin-top: 12px;">
      <n-checkbox
        :checked="draft.castShadow"
        @update:checked="onCastShadow"
      />
    </n-form-item>
  </n-form>
</template>

<script setup>
import { reactive, watch } from 'vue'
import { NForm, NFormItem, NInputNumber, NColorPicker, NCheckbox } from 'naive-ui'
import { sceneStore } from '../../store/sceneStore'

const props = defineProps({
  component: { type: Object, required: true },
})

// Local draft mirrors the relevant subset of the component prop; v-model
// binds here, never on props. Re-syncs from the store after the engine
// commits so clamped/normalized values show up in the UI.
const draft = reactive({
  color:               [...(props.component.color ?? [1, 1, 1])],
  intensity:           props.component.intensity ?? 0,
  castShadow:          props.component.castShadow ?? true,
  useColorTemperature: props.component.useColorTemperature ?? false,
  colorTemperature:    props.component.colorTemperature ?? 5780,
})

watch(
  () => props.component,
  (next) => {
    draft.color               = [...(next.color ?? [1, 1, 1])]
    draft.intensity           = next.intensity ?? 0
    draft.castShadow          = next.castShadow ?? true
    draft.useColorTemperature = next.useColorTemperature ?? false
    draft.colorTemperature    = next.colorTemperature ?? 5780
  },
  { deep: true },
)

// #RRGGBB is the native <input type="color"> wire format, not a theme
// surface — the fallback keeps the picker sensible when draft.color is
// absent or malformed.
const rgbArrayToHex = (rgb) => {
  if (!rgb) return '#ffffff'
  const byte = (v) => Math.max(0, Math.min(255, Math.round(v * 255))).toString(16).padStart(2, '0')
  return `#${byte(rgb[0])}${byte(rgb[1])}${byte(rgb[2])}`
}

const hexToRgbArray = (hex) => {
  const r = parseInt(hex.slice(1, 3), 16) / 255
  const g = parseInt(hex.slice(3, 5), 16) / 255
  const b = parseInt(hex.slice(5, 7), 16) / 255
  return [r, g, b]
}

const commit = (property, value) => {
  const id = sceneStore.selectedEntity?.id
  if (id === undefined) return
  sceneStore.updateProperty({
    id,
    component: 'LightComponent',
    property,
    value,
  }).catch(e => console.error('LightEditor.commit:', e))
}

const onColor = (hex) => {
  const next = hexToRgbArray(hex)
  draft.color = next
  // Engine flips m_UseColorTemperature off as a side-effect of the color
  // setter; mirror that locally so the K-mode checkbox / K input update
  // before the next GET arrives.
  draft.useColorTemperature = false
  commit('color', next)
}

const onIntensity = (value) => {
  draft.intensity = value
  commit('intensity', value)
}

const onCastShadow = (value) => {
  draft.castShadow = value
  commit('castShadow', value)
}

const onUseColorTemperature = (value) => {
  draft.useColorTemperature = value
  commit('useColorTemperature', value)
}

const onColorTemperature = (value) => {
  draft.colorTemperature = value
  commit('colorTemperature', value)
}
</script>
