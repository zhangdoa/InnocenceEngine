<template>
  <n-form label-placement="left" label-width="75" size="small" :show-feedback="false">
    <n-form-item label="Position">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="(val, i) in component.pos" :key="'pos-'+i">
          <n-input-number 
            v-model:value="component.pos[i]" 
            @update:value="update('pos')" 
            :show-button="false"
          >
            <template #prefix>
              <n-text depth="3" :style="{ fontSize: '10px', color: i === 0 ? 'var(--ctp-red)' : i === 1 ? 'var(--ctp-green)' : 'var(--ctp-blue)' }">
                {{ ['X', 'Y', 'Z'][i] }}
              </n-text>
            </template>
          </n-input-number>
        </n-grid-item>
      </n-grid>
    </n-form-item>
    
    <n-form-item label="Rotation" style="margin-top: 12px;">
      <n-text depth="3" style="font-size: 10px; font-family: monospace;">
        {{ component.rot.map(v => v.toFixed(3)).join(', ') }}
      </n-text>
    </n-form-item>

    <n-form-item label="Scale" style="margin-top: 12px;">
      <n-grid :cols="3" :x-gap="6">
        <n-grid-item v-for="(val, i) in component.scale" :key="'scale-'+i">
          <n-input-number 
            v-model:value="component.scale[i]" 
            @update:value="update('scale')" 
            :show-button="false"
          >
            <template #prefix>
              <n-text depth="3" style="font-size: 10px;">{{ ['X', 'Y', 'Z'][i] }}</n-text>
            </template>
          </n-input-number>
        </n-grid-item>
      </n-grid>
    </n-form-item>
  </n-form>
</template>

<script setup>
import { NForm, NFormItem, NGrid, NGridItem, NInputNumber, NText } from 'naive-ui'
import { sceneStore } from '../../store/sceneStore'

const props = defineProps({
  component: {
    type: Object,
    required: true
  }
})

const update = (property) => {
  sceneStore.updateProperty({
    id: sceneStore.selectedEntity.id,
    component: 'TransformComponent',
    property,
    value: props.component[property]
  })
}
</script>
