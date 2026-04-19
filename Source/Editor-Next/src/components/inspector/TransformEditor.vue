<template>
  <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
    <n-form-item label="Position">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="i in [0, 1, 2]" :key="'pos-' + i">
          <n-input-number
            :value="draft.pos[i]"
            @update:value="(v) => onAxis('pos', i, v)"
            :show-button="false"
          >
            <template #prefix>
              <n-text depth="3" :style="{ fontSize: '10px', color: AXIS_COLOR[i] }">
                {{ AXIS[i] }}
              </n-text>
            </template>
          </n-input-number>
        </n-grid-item>
      </n-grid>
    </n-form-item>

    <n-form-item label="Rotation" style="margin-top: 12px;">
      <n-text depth="3" style="font-size: 10px; font-family: monospace;">
        {{ draft.rot.map(v => v.toFixed(3)).join(', ') }}
      </n-text>
    </n-form-item>

    <n-form-item label="Scale" style="margin-top: 12px;">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="i in [0, 1, 2]" :key="'scale-' + i">
          <n-input-number
            :value="draft.scale[i]"
            @update:value="(v) => onAxis('scale', i, v)"
            :show-button="false"
          >
            <template #prefix>
              <n-text depth="3" style="font-size: 10px;">{{ AXIS[i] }}</n-text>
            </template>
          </n-input-number>
        </n-grid-item>
      </n-grid>
    </n-form-item>
  </n-form>
</template>

<script setup>
import { reactive, watch } from 'vue'
import { NForm, NFormItem, NGrid, NGridItem, NInputNumber, NText } from 'naive-ui'
import { sceneStore } from '../../store/sceneStore'

const AXIS = ['X', 'Y', 'Z']
const AXIS_COLOR = ['var(--ctp-red)', 'var(--ctp-green)', 'var(--ctp-blue)']

const props = defineProps({
  component: { type: Object, required: true },
})

// Local draft; v-model writes here, never on props. Re-syncs from the store
// after the engine reply (which may clamp/normalize what the user typed).
const draft = reactive({
  pos:   [...(props.component.pos   ?? [0, 0, 0])],
  rot:   [...(props.component.rot   ?? [0, 0, 0, 1])],
  scale: [...(props.component.scale ?? [1, 1, 1])],
})

watch(
  () => props.component,
  (next) => {
    draft.pos   = [...(next.pos   ?? [0, 0, 0])]
    draft.rot   = [...(next.rot   ?? [0, 0, 0, 1])]
    draft.scale = [...(next.scale ?? [1, 1, 1])]
  },
  { deep: true },
)

const onAxis = (property, index, value) => {
  const nextArr = [...draft[property]]
  nextArr[index] = value
  draft[property] = nextArr
  const id = sceneStore.selectedEntity?.id
  if (id === undefined) return
  sceneStore.updateProperty({
    id,
    component: 'TransformComponent',
    property,
    value: nextArr,
  }).catch(e => console.error('TransformEditor.onAxis:', e))
}
</script>
