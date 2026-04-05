<template>
  <div class="viewport-panel">
    <div class="viewport-info" v-if="sharedHandle">
      Handle: 0x{{ sharedHandle.toString(16).toUpperCase() }}
    </div>
    <canvas ref="viewportCanvas" class="viewport-canvas"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, defineProps, watch } from 'vue'

const props = defineProps({
  sharedHandle: BigInt,
})

const viewportCanvas = ref(null)

onMounted(() => {
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
