<template>
  <div class="editor-container">
    <header class="editor-header">
      <div class="logo">InnocenceEngine</div>
      <div class="status" :class="{ connected: isConnected }">
        {{ isConnected ? 'Connected' : 'Disconnected' }}
      </div>
    </header>
    <main class="editor-main">
      <aside class="sidebar left">
        <h3>Scene Tree</h3>
        <ul>
          <li v-for="entity in entities" :key="entity.id">{{ entity.name }}</li>
        </ul>
      </aside>
      <section class="viewport">
        <!-- Engine output will go here -->
        <div class="viewport-placeholder">Engine Viewport</div>
      </section>
      <aside class="sidebar right">
        <h3>Properties</h3>
      </aside>
    </main>
    <footer class="editor-footer">
      <div class="console">Console: Running...</div>
    </footer>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const isConnected = ref(false)
const entities = ref([
  { id: 1, name: 'Main Camera' },
  { id: 2, name: 'Directional Light' },
  { id: 3, name: 'Sponza Palace' }
])
</script>

<style scoped>
.editor-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.editor-header {
  height: 40px;
  background: #2d2d2d;
  display: flex;
  align-items: center;
  padding: 0 10px;
  justify-content: space-between;
}

.connected { color: #42b983; }

.editor-main {
  flex: 1;
  display: flex;
}

.sidebar {
  width: 250px;
  background: #252526;
  border: 1px solid #333;
}

.viewport {
  flex: 1;
  background: #000;
  display: flex;
  align-items: center;
  justify-content: center;
}

.editor-footer {
  height: 30px;
  background: #007acc;
  padding: 0 10px;
  display: flex;
  align-items: center;
  font-size: 12px;
}
</style>
