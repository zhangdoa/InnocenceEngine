<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import {
  NScrollbar,
  NEmpty,
  NSpace,
  NText,
  NButton,
  NSwitch,
  NDivider,
} from 'naive-ui'
import { taskGraphStore } from '../store/taskGraphStore'
import { connectionStore } from '../store/connectionStore'

const autoRefresh = ref(true)
const REFRESH_MS = 500
let timer = null

const startTimer = () => {
  stopTimer()
  if (!autoRefresh.value) return
  timer = setInterval(() => taskGraphStore.refresh(), REFRESH_MS)
}

const stopTimer = () => {
  if (timer) clearInterval(timer)
  timer = null
}

watch(autoRefresh, startTimer)
watch(
  () => connectionStore.isConnected,
  (connected) => {
    if (connected) {
      taskGraphStore.refresh()
      startTimer()
    } else {
      stopTimer()
      taskGraphStore.clear()
    }
  },
)

onMounted(() => {
  if (connectionStore.isConnected) {
    taskGraphStore.refresh()
    startTimer()
  }
})

onUnmounted(stopTimer)

// 100ns ticks (Win32 QPC-derived) → ms.
const ticksToMs = (ticks) => Number(ticks) / 10000

const tasksByThread = computed(() =>
  taskGraphStore.threads.map((t) => ({
    index: t.index,
    reports: [...(t.reports || [])]
      .filter((r) => r.startTime && r.finishTime && r.finishTime >= r.startTime)
      .sort((a, b) => Number(b.startTime) - Number(a.startTime))
      .slice(0, 12)
      .map((r) => ({
        name: r.name,
        durationMs: ticksToMs(BigInt(r.finishTime) - BigInt(r.startTime)),
      })),
  })),
)

const longestDuration = computed(() => {
  let max = 0
  for (const t of tasksByThread.value)
    for (const r of t.reports) if (r.durationMs > max) max = r.durationMs
  return Math.max(max, 1)
})
</script>

<template>
  <div class="task-debugger" data-test="task-debugger-panel">
    <div v-if="!connectionStore.isConnected" class="empty-container">
      <n-empty description="Engine offline" size="small" />
    </div>

    <template v-else>
      <div class="toolbar">
        <n-space align="center" :size="12">
          <n-text depth="3" class="section-label">Recent task reports</n-text>
          <n-space align="center" :size="6">
            <n-text depth="3" style="font-size: 11px">Auto</n-text>
            <n-switch v-model:value="autoRefresh" size="small" data-test="task-auto" />
          </n-space>
        </n-space>
        <n-button size="tiny" quaternary data-test="task-refresh" @click="taskGraphStore.refresh">
          Refresh
        </n-button>
      </div>

      <n-scrollbar>
        <div v-for="thread in tasksByThread" :key="thread.index" class="thread-block">
          <n-text strong>Thread {{ thread.index }}</n-text>
          <n-empty
            v-if="!thread.reports.length"
            description="No reports yet"
            size="small"
            style="margin: 4px 0"
          />
          <div v-else class="report-list">
            <div
              v-for="(report, idx) in thread.reports"
              :key="idx"
              class="report-row"
              :title="report.name"
            >
              <span class="report-name">{{ report.name }}</span>
              <div class="bar-track">
                <div
                  class="bar-fill"
                  :style="{ width: ((report.durationMs / longestDuration) * 100).toFixed(1) + '%' }"
                />
              </div>
              <span class="report-duration">{{ report.durationMs.toFixed(2) }} ms</span>
            </div>
          </div>
          <n-divider style="margin: 6px 0" />
        </div>
      </n-scrollbar>
    </template>
  </div>
</template>

<style scoped>
.task-debugger {
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

.toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--ctp-surface0);
}

.section-label {
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.thread-block {
  padding: 8px 12px;
}

.report-list {
  margin-top: 4px;
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.report-row {
  display: grid;
  grid-template-columns: minmax(120px, 2fr) 3fr 80px;
  gap: 8px;
  align-items: center;
  font-size: 12px;
}

.report-name {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.bar-track {
  height: 6px;
  background: var(--ctp-surface0);
  border-radius: 3px;
  overflow: hidden;
}

.bar-fill {
  height: 100%;
  background: var(--ctp-blue);
}

.report-duration {
  text-align: right;
  font-variant-numeric: tabular-nums;
  color: var(--ctp-subtext0);
}
</style>
