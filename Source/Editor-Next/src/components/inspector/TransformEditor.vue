<template>
  <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
    <n-form-item label="Position">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="i in [0, 1, 2]" :key="'pos-' + i">
          <n-input-number
            :value="draft.pos[i]"
            @update:value="(v) => onPosAxis(i, v)"
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
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="i in [0, 1, 2]" :key="'rot-' + i">
          <n-input-number
            :value="draft.rotEuler[i]"
            @update:value="(v) => onRotAxis(i, v)"
            @focus="focusedRotAxis = i"
            @blur="focusedRotAxis = null"
            :show-button="false"
            :precision="2"
            data-test-prop="rot-euler-input"
            :data-test-axis="AXIS[i]"
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

    <n-form-item label="Scale" style="margin-top: 12px;">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="i in [0, 1, 2]" :key="'scale-' + i">
          <n-input-number
            :value="draft.scale[i]"
            @update:value="(v) => onScaleAxis(i, v)"
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
import { reactive, ref, watch } from 'vue'
import { NForm, NFormItem, NGrid, NGridItem, NInputNumber, NText } from 'naive-ui'
import { sceneStore } from '../../store/sceneStore'
import { eulerDegToQuat, quatToEulerDeg } from '../../math/quatEuler'

const AXIS = ['X', 'Y', 'Z']
const AXIS_COLOR = ['var(--ctp-red)', 'var(--ctp-green)', 'var(--ctp-blue)']

const props = defineProps({
  component: { type: Object, required: true },
})

const draft = reactive({
  pos:      [...(props.component.pos   ?? [0, 0, 0])],
  rotEuler: quatToEulerDeg(props.component.rot ?? [0, 0, 0, 1]),
  scale:    [...(props.component.scale ?? [1, 1, 1])],
})

// Null when no axis is being edited; 0/1/2 while a rotation input is focused.
// Server replies mid-edit overwrite the input's external value with the
// re-decomposed quat, which Naive's NInputNumber surfaces as a cursor jump
// and a strikethrough on the digit being typed. Freezing the focused axis
// keeps typing stable; the next blur lets the server state land.
const focusedRotAxis = ref(null)

watch(
  () => props.component,
  (next) => {
    draft.pos   = [...(next.pos   ?? [0, 0, 0])]
    draft.scale = [...(next.scale ?? [1, 1, 1])]
    const fresh = quatToEulerDeg(next.rot ?? [0, 0, 0, 1])
    const fi = focusedRotAxis.value
    draft.rotEuler = fi == null
      ? fresh
      : draft.rotEuler.map((v, i) => (i === fi ? v : fresh[i]))
  },
  { deep: true },
)

const commit = (property, value) => {
  const id = sceneStore.selectedEntity?.id
  if (id === undefined) return
  sceneStore
    .updateProperty({ id, component: 'TransformComponent', property, value })
    .catch((e) => console.error('TransformEditor.commit:', e))
}

const onPosAxis = (index, value) => {
  const next = [...draft.pos]
  next[index] = value
  draft.pos = next
  commit('pos', next)
}

const onScaleAxis = (index, value) => {
  const next = [...draft.scale]
  next[index] = value
  draft.scale = next
  commit('scale', next)
}

const onRotAxis = (index, value) => {
  const nextEuler = [...draft.rotEuler]
  nextEuler[index] = value
  draft.rotEuler = nextEuler
  commit('rot', eulerDegToQuat(nextEuler))
}
</script>
