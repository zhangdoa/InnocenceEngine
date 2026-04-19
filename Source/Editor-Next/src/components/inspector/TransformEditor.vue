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

// While a rotation input is focused we hold the draft's authoritative Euler
// untouched; the null-case handler lets server echoes land normally.
const focusedRotAxis = ref(null)

// Queue of quats we committed locally but haven't yet seen echoed back. Each
// reply for our own write should match the head of this queue — we drop it
// and skip the Euler re-derivation, so repeated edits can't accumulate
// quat↔Euler round-trip drift (gimbal branches flipping axes on us) as the
// user types. A quat that doesn't match any pending entry is treated as an
// external mutation and re-syncs the draft.
const pendingQuatEchoes = []
const QUAT_ECHO_TOL = 1e-5
const sameQuat = (a, b) =>
  !!a && !!b &&
  Math.abs(a[0] - b[0]) < QUAT_ECHO_TOL &&
  Math.abs(a[1] - b[1]) < QUAT_ECHO_TOL &&
  Math.abs(a[2] - b[2]) < QUAT_ECHO_TOL &&
  Math.abs(a[3] - b[3]) < QUAT_ECHO_TOL

watch(
  () => props.component,
  (next) => {
    draft.pos   = [...(next.pos   ?? [0, 0, 0])]
    draft.scale = [...(next.scale ?? [1, 1, 1])]

    const fresh = next.rot ?? [0, 0, 0, 1]
    const echoIdx = pendingQuatEchoes.findIndex((q) => sameQuat(q, fresh))
    if (echoIdx >= 0) {
      pendingQuatEchoes.splice(0, echoIdx + 1)
      return
    }

    const freshEuler = quatToEulerDeg(fresh)
    const fi = focusedRotAxis.value
    draft.rotEuler = fi == null
      ? freshEuler
      : draft.rotEuler.map((v, i) => (i === fi ? v : freshEuler[i]))
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
  const quat = eulerDegToQuat(nextEuler)
  pendingQuatEchoes.push(quat)
  commit('rot', quat)
}
</script>
