<script setup>
import { computed, watch, ref } from 'vue'
import {
  NScrollbar,
  NEmpty,
  NSelect,
  NButton,
  NSpace,
  NText,
  NTag,
  useMessage,
} from 'naive-ui'
import { renderTargetStore } from '../store/renderTargetStore'
import { connectionStore } from '../store/connectionStore'

const message = useMessage()
const selectedKey = ref(null)

// Store self-refreshes on connect/disconnect via its own on('engine-connected')
// subscription — the panel just renders the store.

// Flatten the (pass, rt) hierarchy into a single Naive select with
// composite values 'passName::rtIndex'. Each option's label shows
// "PassName / RTName".
const options = computed(() => {
  const out = []
  for (const pass of renderTargetStore.passes) {
    for (const rt of pass.targets) {
      out.push({
        label: `${pass.name} / ${rt.name}`,
        value: `${pass.name}::${rt.index}`,
      })
    }
  }
  return out
})

const overrideLabel = computed(() => {
  const o = renderTargetStore.override
  if (!o) return null
  const pass = renderTargetStore.passes.find((p) => p.name === o.pass)
  if (!pass) return `${o.pass} / [${o.rtIndex}]`
  const rt = pass.targets.find((t) => t.index === o.rtIndex)
  return rt ? `${o.pass} / ${rt.name}` : `${o.pass} / [${o.rtIndex}]`
})

watch(
  () => renderTargetStore.override,
  (o) => {
    selectedKey.value = o ? `${o.pass}::${o.rtIndex}` : null
  },
  { immediate: true },
)

const onApply = () => {
  if (!selectedKey.value) {
    message.warning('Pick a render target first')
    return
  }
  const [pass, idxStr] = selectedKey.value.split('::')
  renderTargetStore.setOverride(pass, parseInt(idxStr, 10))
  message.success(`Viewport now showing ${selectedKey.value.replace('::', ' / RT')}`)
}

const onReset = () => {
  renderTargetStore.clearOverride()
  selectedKey.value = null
  message.info('Viewport reset to default source')
}
</script>

<template>
  <div class="rt-debugger" data-test="rt-debugger-panel">
    <div v-if="!connectionStore.isConnected" class="empty-container">
      <n-empty description="Engine offline" size="small" />
    </div>

    <n-scrollbar v-else>
      <div class="header-row">
        <n-text depth="3" class="section-label">Override viewport source</n-text>
        <n-tag v-if="overrideLabel" type="warning" size="small" round>
          {{ overrideLabel }}
        </n-tag>
        <n-tag v-else type="success" size="small" round>default</n-tag>
      </div>

      <n-empty
        v-if="!options.length"
        description="No render targets reported yet"
        size="small"
        style="margin: 12px"
      />

      <div v-else class="picker-row">
        <n-select
          v-model:value="selectedKey"
          :options="options"
          placeholder="Select pass / RT…"
          size="small"
          filterable
          data-test="rt-picker"
        />
        <n-space :size="6" class="actions">
          <n-button
            size="small"
            type="primary"
            :disabled="!selectedKey"
            data-test="rt-apply"
            @click="onApply"
          >
            Use as viewport
          </n-button>
          <n-button
            size="small"
            :disabled="!renderTargetStore.override"
            data-test="rt-reset"
            @click="onReset"
          >
            Reset
          </n-button>
          <n-button size="small" quaternary data-test="rt-refresh" @click="renderTargetStore.refresh">
            Refresh list
          </n-button>
        </n-space>
      </div>
    </n-scrollbar>
  </div>
</template>

<style scoped>
.rt-debugger {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.empty-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.header-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--ctp-surface0);
}

.section-label {
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.picker-row {
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.actions {
  flex-wrap: wrap;
}
</style>
