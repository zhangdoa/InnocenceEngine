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

      <div v-for="comp in selectedEntity.components" :key="comp.type" class="component-section">
        <div class="component-header">{{ comp.type }}</div>
        
        <!-- Transform Component -->
        <template v-if="comp.type === 'TransformComponent'">
          <div class="property-group">
            <label>Position</label>
            <div class="vector3">
              <div class="input-field"><span>X</span><input type="number" v-model="comp.pos[0]" @change="updateProp(comp.type, 'pos', comp.pos)" step="0.1" /></div>
              <div class="input-field"><span>Y</span><input type="number" v-model="comp.pos[1]" @change="updateProp(comp.type, 'pos', comp.pos)" step="0.1" /></div>
              <div class="input-field"><span>Z</span><input type="number" v-model="comp.pos[2]" @change="updateProp(comp.type, 'pos', comp.pos)" step="0.1" /></div>
            </div>
          </div>
          <div class="property-group">
            <label>Scale</label>
            <div class="vector3">
              <div class="input-field"><span>X</span><input type="number" v-model="comp.scale[0]" @change="updateProp(comp.type, 'scale', comp.scale)" step="0.1" /></div>
              <div class="input-field"><span>Y</span><input type="number" v-model="comp.scale[1]" @change="updateProp(comp.type, 'scale', comp.scale)" step="0.1" /></div>
              <div class="input-field"><span>Z</span><input type="number" v-model="comp.scale[2]" @change="updateProp(comp.type, 'scale', comp.scale)" step="0.1" /></div>
            </div>
          </div>
        </template>

        <!-- Light Component -->
        <template v-else-if="comp.type === 'LightComponent'">
          <div class="property-group">
            <label>Color (RGB)</label>
            <div class="vector3">
              <div class="input-field"><span>R</span><input type="number" v-model="comp.color[0]" @change="updateProp(comp.type, 'color', comp.color)" min="0" max="1" step="0.05" /></div>
              <div class="input-field"><span>G</span><input type="number" v-model="comp.color[1]" @change="updateProp(comp.type, 'color', comp.color)" min="0" max="1" step="0.05" /></div>
              <div class="input-field"><span>B</span><input type="number" v-model="comp.color[2]" @change="updateProp(comp.type, 'color', comp.color)" min="0" max="1" step="0.05" /></div>
            </div>
          </div>
          <div class="property-group">
            <label>Intensity (lm)</label>
            <div class="input-field full">
              <input type="number" v-model="comp.intensity" @change="updateProp(comp.type, 'intensity', comp.intensity)" step="10" />
            </div>
          </div>
        </template>

        <div v-else class="no-props">Component properties not yet implemented</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue'

const props = defineProps({
  selectedEntity: Object,
  onUpdateProperty: Function
})

const updateProp = (component, property, value) => {
  if (props.onUpdateProperty) {
    props.onUpdateProperty({
      id: props.selectedEntity.id,
      component,
      property,
      value
    })
  }
}
</script>

<style scoped>
.property-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #252526;
  color: #ccc;
  overflow-y: auto;
  font-family: sans-serif;
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
  font-size: 14px;
  margin: 0;
  color: #fff;
}

.entity-info .id {
  font-size: 9px;
  opacity: 0.4;
}

.component-section {
  margin-bottom: 15px;
  background: #2d2d2d;
  border-radius: 4px;
  overflow: hidden;
  border: 1px solid #333;
}

.component-header {
  padding: 6px 10px;
  background: #37373d;
  font-size: 11px;
  font-weight: bold;
  color: #eee;
  border-bottom: 1px solid #333;
}

.property-group {
  padding: 8px 10px;
}

.property-group label {
  display: block;
  font-size: 10px;
  margin-bottom: 4px;
  opacity: 0.6;
  text-transform: uppercase;
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

.input-field.full { width: 100%; }

.input-field span {
  font-size: 9px;
  font-weight: bold;
  margin-right: 4px;
  opacity: 0.3;
  width: 10px;
  text-align: center;
}

.input-field input {
  width: 100%;
  background: transparent;
  border: none;
  color: #fff;
  font-size: 11px;
  outline: none;
}

.input-field input::-webkit-inner-spin-button,
.input-field input::-webkit-outer-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.no-props {
  padding: 10px;
  font-size: 10px;
  opacity: 0.4;
  text-align: center;
}
</style>
