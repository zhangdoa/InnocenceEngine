<template>
  <div class="viewport-panel">
    <div class="viewport-info" v-if="props.params?.sharedHandle">
      Handle: 0x{{ props.params.sharedHandle.toString(16).toUpperCase() }}
    </div>
    <canvas ref="viewportCanvas" class="viewport-canvas"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, defineProps, watch } from 'vue'

const props = defineProps({
  params: Object
})

const viewportCanvas = ref(null)

onMounted(() => {
  console.log('ViewportPanel mounted');
  if (window.require) {
    try {
      const { sharedTexture, ipcRenderer } = window.require('electron')
      console.log('ViewportPanel: Got sharedTexture from electron', !!sharedTexture);
      
      if (sharedTexture && sharedTexture.setSharedTextureReceiver) {
        console.log('ViewportPanel: Setting receiver...');
        sharedTexture.setSharedTextureReceiver(async (data) => {
          console.log('ViewportPanel: Received shared texture data', !!data.importedSharedTexture);
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
        console.log('ViewportPanel: Receiver set.');
      } else {
        console.warn('ViewportPanel: sharedTexture or setSharedTextureReceiver is missing!', sharedTexture);
      }
      
      // Request texture immediately if handle is already there
      if (props.params && props.params.sharedHandle) {
        console.log('ViewportPanel: Initial handle present. Asking main process to send texture.');
        ipcRenderer.send('renderer-ready-for-texture')
      }

      // Also listen to IPC directly to know when the texture is imported by main
      ipcRenderer.on('viewport-ready', (event, info) => {
        console.log('ViewportPanel: Received viewport-ready from main. Asking main process to send texture.');
        ipcRenderer.send('renderer-ready-for-texture')
      })
      
    } catch (e) {
      console.error('ViewportPanel: Error setting up shared texture:', e);
    }
  } else {
    console.warn('ViewportPanel: window.require is not available!');
  }
})
</script>

<style scoped>
.viewport-panel {
  width: 100%;
  height: 100%;
  background: #000;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

.viewport-canvas {
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
}

.viewport-info {
  position: absolute;
  top: 5px;
  left: 5px;
  background: rgba(0,0,0,0.6);
  padding: 2px 8px;
  border-radius: 3px;
  font-size: 10px;
  color: #aaa;
  z-index: 10;
  pointer-events: none;
}
</style>
