<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { NEmpty, NIcon, NText, NSpace, NButton } from 'naive-ui'
import { VideocamOutline, AlertCircleOutline } from '@vicons/ionicons5'
import { useIpc, request } from '../composables/useIpc'
import { connectionStore } from '../store/connectionStore'

// Viewport state machine
//
//   pending → live (on VIEWPORT_READY)
//   pending → failed (on VIEWPORT_FAILED)
//   live    → failed (on late VIEWPORT_FAILED)
//   live    → pending (on connection disconnect)
//   failed  → pending (on manual retry)
//
// Engine frames are composited into this pane via Electron's sharedTexture
// API from main.js. This component's job is presentational: track state,
// size the canvas, propagate the layout-driven back-buffer size request.
const status = ref('pending') // 'pending' | 'live' | 'failed'
const dims = ref({ width: 0, height: 0 })
const failureReason = ref('')

const containerRef = ref(null)
const canvasRef = ref(null)

const RESIZE_DEBOUNCE_MS = 150
let resizeTimer = null
let resizeObserver = null

const resolutionLabel = computed(() =>
  status.value === 'live' && dims.value.width && dims.value.height
    ? `${dims.value.width} × ${dims.value.height}`
    : '—',
)

const statusLabel = computed(() => {
  switch (status.value) {
    case 'live':    return 'LIVE'
    case 'failed':  return 'FAILED'
    default:        return 'WAITING'
  }
})

const { on } = useIpc()

on('VIEWPORT_READY', (payload) => {
  status.value = 'live'
  dims.value = {
    width:  payload?.width  ?? dims.value.width,
    height: payload?.height ?? dims.value.height,
  }
  failureReason.value = ''
})

on('VIEWPORT_FAILED', (payload) => {
  status.value = 'failed'
  failureReason.value = payload?.reason || 'Viewport bind failed'
})

on('engine-connected', ({ connected }) => {
  if (!connected) {
    status.value = 'pending'
    failureReason.value = ''
  }
})

function scheduleResize() {
  if (resizeTimer) clearTimeout(resizeTimer)
  resizeTimer = setTimeout(() => {
    resizeTimer = null
    const el = containerRef.value
    if (!el) return
    const rect = el.getBoundingClientRect()
    const width  = Math.max(1, Math.floor(rect.width))
    const height = Math.max(1, Math.floor(rect.height))
    if (!connectionStore.isConnected) return
    // Fire-and-degrade: request_VIEWPORT_RESIZE may be a no-op on the
    // engine side until TASK-73 lands real swap-chain resize. The reply
    // still echoes dims so we update our local canvas.
    request('VIEWPORT_RESIZE', { width, height }, { timeoutMs: 5000 })
      .then((result) => {
        if (result?.width && result?.height) {
          dims.value = { width: result.width, height: result.height }
        }
      })
      .catch((e) => {
        if (e?.code !== 'NO_HANDLER' && e?.code !== 'DISCONNECTED') {
          console.warn('ViewportPanel: VIEWPORT_RESIZE rejected:', e?.message || e)
        }
      })
  }, RESIZE_DEBOUNCE_MS)
}

function retry() {
  status.value = 'pending'
  failureReason.value = ''
  if (typeof window !== 'undefined' && window.require) {
    const { ipcRenderer } = window.require('electron')
    ipcRenderer.send('engine-retry')
  }
}

onMounted(() => {
  if (containerRef.value && typeof ResizeObserver !== 'undefined') {
    resizeObserver = new ResizeObserver(() => scheduleResize())
    resizeObserver.observe(containerRef.value)
  }
})

onBeforeUnmount(() => {
  if (resizeTimer) { clearTimeout(resizeTimer); resizeTimer = null }
  if (resizeObserver) { resizeObserver.disconnect(); resizeObserver = null }
})
</script>

<template>
  <div class="viewport-panel" data-test="viewport-panel" :data-test-status="status">
    <div v-if="status === 'pending'" class="overlay">
      <n-empty description="Waiting for engine viewport…" size="large">
        <template #icon>
          <n-icon><videocam-outline /></n-icon>
        </template>
      </n-empty>
    </div>

    <div v-else-if="status === 'failed'" class="overlay">
      <n-empty :description="failureReason" size="large">
        <template #icon>
          <n-icon><alert-circle-outline /></n-icon>
        </template>
        <template #extra>
          <n-button size="small" type="primary" data-test="viewport-retry" @click="retry">
            Retry
          </n-button>
        </template>
      </n-empty>
    </div>

    <div class="viewport-container" ref="containerRef">
      <canvas
        ref="canvasRef"
        class="viewport-canvas"
        :width="dims.width || 1"
        :height="dims.height || 1"
        data-test="viewport-canvas"
      />

      <div class="viewport-ui" v-if="status === 'live'">
        <n-space vertical align="end" class="stats">
          <n-text depth="3" style="font-size: 10px;" data-test="viewport-status">
            {{ statusLabel }}
          </n-text>
          <n-text depth="3" style="font-size: 10px;" data-test="viewport-resolution">
            {{ resolutionLabel }}
          </n-text>
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
  /* The engine's shared texture composites into this pane; the fallback
   * surface shows only before VIEWPORT_READY. Use the darkest Catppuccin
   * token so the fallback matches the palette in every flavor. */
  background: var(--ctp-crust);
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
  background: color-mix(in srgb, var(--ctp-crust) 65%, transparent);
  padding: 4px 8px;
  border-radius: 4px;
}
</style>
