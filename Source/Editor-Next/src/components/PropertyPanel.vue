<template>
  <div class="property-panel">
    <div class="panel-header">Properties</div>
    <div v-if="!selectedEntity" class="no-selection">
      Select an entity to view properties
    </div>
    <div v-else class="properties-content">
      <div class="entity-info">
        <h2>{{ selectedEntity.name }}</h2>
        <span class="id">ID: {{ selectedEntity.id }}</span>
      </div>

      <!-- Transform Component -->
      <div class="component-section">
        <div class="component-header">Transform</div>
        <div class="property-group">
          <label>Position</label>
          <div class="vector3">
            <div class="input-field"><span>X</span><input type="number" v-model="transform.pos.x" /></div>
            <div class="input-field"><span>Y</span><input type="number" v-model="transform.pos.y" /></div>
            <div class="input-field"><span>Z</span><input type="number" v-model="transform.pos.z" /></div>
          </div>
        </div>
        <div class="property-group">
          <label>Rotation</label>
          <div class="vector3">
            <div class="input-field"><span>X</span><input type="number" v-model="transform.rot.x" /></div>
            <div class="input-field"><span>Y</span><input type="number" v-model="transform.rot.y" /></div>
            <div class="input-field"><span>Z</span><input type="number" v-model="transform.rot.z" /></div>
          </div>
        </div>
        <div class="property-group">
          <label>Scale</label>
          <div class="vector3">
            <div class="input-field"><span>X</span><input type="number" v-model="transform.scale.x" /></div>
            <div class="input-field"><span>Y</span><input type="number" v-model="transform.scale.y" /></div>
            <div class="input-field"><span>Z</span><input type="number" v-model="transform.scale.z" /></div>
          </div>
        </div>
      </div>

      <!-- Placeholder for other components -->
      <div v-for="comp in components" :key="comp.type" class="component-section">
        <div class="component-header">{{ comp.type }}</div>
        <div class="no-props">Component data loading...</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, defineProps, reactive, watch } from 'vue'

const props = defineProps({
  selectedEntity: Object
})

const transform = reactive({
  pos: { x: 0, y: 0, z: 0 },
  rot: { x: 0, y: 0, z: 0 },
  scale: { x: 1, y: 1, z: 1 }
})

const components = ref([
  { type: 'MeshComponent' },
  { type: 'MaterialComponent' }
])

watch(() => props.selectedEntity, (newVal) => {
  if (newVal) {
    // In a real impl, we would fetch this data from the engine
    console.log('Fetching properties for', newVal.id)
  }
})
</script>

<style scoped>
.property-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #252526;
  color: #ccc;
  overflow-y: auto;
}

.panel-header {
  padding: 8px 12px;
  background: #2d2d2d;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid #333;
}

.no-selection {
  padding: 40px 20px;
  text-align: center;
  font-style: italic;
  opacity: 0.5;
}

.properties-content {
  padding: 12px;
}

.entity-info {
  margin-bottom: 20px;
  padding-bottom: 10px;
  border-bottom: 1px solid #333;
}

.entity-info h2 {
  font-size: 16px;
  margin: 0;
  color: #fff;
}

.entity-info .id {
  font-size: 10px;
  opacity: 0.5;
}

.component-section {
  margin-bottom: 15px;
  background: #2d2d2d;
  border-radius: 4px;
  overflow: hidden;
}

.component-header {
  padding: 6px 10px;
  background: #37373d;
  font-size: 12px;
  font-weight: bold;
  color: #eee;
}

.property-group {
  padding: 8px 10px;
}

.property-group label {
  display: block;
  font-size: 11px;
  margin-bottom: 4px;
  opacity: 0.8;
}

.vector3 {
  display: flex;
  gap: 5px;
}

.input-field {
  flex: 1;
  background: #3c3c3c;
  border-radius: 2px;
  display: flex;
  align-items: center;
  padding: 2px 4px;
}

.input-field span {
  font-size: 9px;
  font-weight: bold;
  margin-right: 4px;
  opacity: 0.5;
  width: 10px;
}

.input-field input {
  width: 100%;
  background: transparent;
  border: none;
  color: #fff;
  font-size: 11px;
  outline: none;
}

.no-props {
  padding: 10px;
  font-size: 11px;
  opacity: 0.5;
}
</style>
