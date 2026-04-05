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
        <div class="viewport-info">
          <p v-if="sharedHandle">Shared Handle: 0x{{ sharedHandle.toString(16).toUpperCase() }}</p>
          <p v-else>Waiting for Shared Handle...</p>
        </div>
        <canvas ref="viewportCanvas" class="viewport-canvas"></canvas>
      </section>
      <aside class="sidebar right">
        <h3>Properties</h3>
      </aside>
    </main>
    <footer class="editor-footer">
      <div class="console">Console: {{ lastMessage || 'Running...' }}</div>
    </footer>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'

const isConnected = ref(false)
const sharedHandle = ref(null)
const lastMessage = ref('')
const viewportCanvas = ref(null)
const entities = ref([
  { id: 1, name: 'Main Camera' },
  { id: 2, name: 'Directional Light' },
  { id: 3, name: 'Sponza Palace' }
])

let socket = null

const connect = () => {
  console.log('Connecting to Engine...')
  socket = new WebSocket('ws://localhost:8081')

  socket.onopen = () => {
    console.log('Connected to Engine')
    isConnected.value = true
    socket.send(JSON.stringify({ type: 'HELO' }))
  }

  socket.onmessage = (event) => {
    lastMessage.value = event.data
    try {
      const msg = JSON.parse(event.data)
      if (msg.type === 'HELLO_REPLY') {
        sharedHandle.value = msg.sharedHandle
      }
    } catch (e) {
      console.error('Failed to parse message:', e)
    }
  }

  socket.onclose = () => {
    isConnected.value = false
    sharedHandle.value = null
    setTimeout(connect, 2000)
  }
}

onMounted(() => {
  connect()

  // Setup Shared Texture Receiver
  if (window.require) {
    const { sharedTexture } = window.require('electron')
    
    sharedTexture.setSharedTextureReceiver(async (data) => {
      const { importedSharedTexture } = data
      
      const videoFrame = importedSharedTexture.getVideoFrame()
      
      if (viewportCanvas.value) {
        const canvas = viewportCanvas.value
        const ctx = canvas.getContext('2d')
        
        if (canvas.width !== videoFrame.displayWidth || canvas.height !== videoFrame.displayHeight) {
          canvas.width = videoFrame.displayWidth
          canvas.height = videoFrame.displayHeight
        }
        
        ctx.drawImage(videoFrame, 0, 0)
      }

      videoFrame.close()
      importedSharedTexture.release()
    })
  }
})

onUnmounted(() => {
  if (socket) {
    socket.close()
  }
})
</script>

<style scoped>
.editor-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
  background: #1e1e1e;
  color: #ccc;
}

.editor-header {
  height: 40px;
  background: #2d2d2d;
  display: flex;
  align-items: center;
  padding: 0 15px;
  justify-content: space-between;
  border-bottom: 1px solid #333;
}

.logo { font-weight: bold; color: #fff; }
.status { font-size: 12px; }
.connected { color: #42b983; }

.editor-main {
  flex: 1;
  display: flex;
}

.sidebar {
  width: 250px;
  background: #252526;
  border: 1px solid #333;
  padding: 10px;
}

.sidebar h3 { font-size: 14px; margin-bottom: 10px; color: #888; }
.sidebar ul { list-style: none; padding: 0; }
.sidebar li { padding: 5px; cursor: pointer; border-radius: 3px; }
.sidebar li:hover { background: #37373d; }

.viewport {
  flex: 1;
  background: #000;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  position: relative;
  overflow: hidden;
}

.viewport-canvas {
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
}

.viewport-info {
  position: absolute;
  top: 10px;
  left: 10px;
  background: rgba(0,0,0,0.5);
  padding: 5px 10px;
  border-radius: 5px;
  font-size: 12px;
  z-index: 10;
}

.editor-footer {
  height: 30px;
  background: #007acc;
  padding: 0 15px;
  display: flex;
  align-items: center;
  font-size: 12px;
  color: #fff;
}
</style>
