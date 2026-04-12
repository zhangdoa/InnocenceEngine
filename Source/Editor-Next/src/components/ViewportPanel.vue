<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { NEmpty, NIcon, NText, NSpace } from 'naive-ui'
import { VideocamOutline } from '@vicons/ionicons5'
import { connectionStore } from '../store/connectionStore'

const canvasRef = ref(null)
const isTextureReady = ref(false)

onMounted(() => {
  console.log('ViewportPanel mounted')
  
  // Listen for shared texture events from main process
  // In this Electron version, shared textures might be bound to the frame
  // or sent via a specific IPC channel that the native layer handles.
})

onUnmounted(() => {
})
</script>

<template>
  <div class="viewport-panel">
    <div v-if="!connectionStore.isConnected" class="overlay">
      <n-empty description="Waiting for Engine..." size="large">
        <template #icon>
          <n-icon><videocam-outline /></n-icon>
        </template>
      </n-empty>
    </div>
    
    <div class="viewport-container" ref="viewportContainer">
      <!-- The shared texture is rendered directly into the window's compositor 
           by Electron's sharedTexture API. We just need to provide a transparent 
           hole or a specific element if the API requires it. -->
      <canvas ref="canvasRef" class="viewport-canvas"></canvas>
      
      <div class="viewport-ui">
        <n-space vertical align="end" class="stats">
          <n-text depth="3" style="font-size: 10px;">DX12 SHARED TEXTURE MODE</n-text>
          <n-text depth="3" style="font-size: 10px;">RESOLUTION: 1280x720</n-text>
        </n-space>
      </div>
    </div>
  </div>
</template>

<style scoped>
.viewport-panel {
  position: relative;
  width: 100%;
  height: 100%;
  background: #000;
  overflow: hidden;
}

.overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 10;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--ctp-base);
}

.viewport-container {
  width: 100%;
  height: 100%;
  position: relative;
}

.viewport-canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.viewport-ui {
  position: absolute;
  top: 10px;
  right: 10px;
  pointer-events: none;
}

.stats {
  background: rgba(0, 0, 0, 0.5);
  padding: 4px 8px;
  border-radius: 4px;
}
</style>
