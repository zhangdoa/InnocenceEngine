<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import {
  NScrollbar,
  NEmpty,
  NSpace,
  NText,
  NButton,
  NSwitch,
} from 'naive-ui'
import { taskGraphStore } from '../store/taskGraphStore'
import { connectionStore } from '../store/connectionStore'
import { uiStore } from '../store/uiStore'

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
      startTimer()
    } else {
      stopTimer()
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

// TaskScheduler stamps reports with Timer::GetCurrentTimeFromEpoch in
// microseconds (see Engine/Common/Thread.cpp). The previous implementation
// divided by 10000, which assumed 100ns ticks — off by 10x. Correct factor
// is 1000 (µs → ms).
const usToMs = (us) => Number(us) / 1000

// Per-thread shape: index, valid reports (sorted newest first), total
// active-time (sum of durations) and per-task durations for the detail
// expansion. `durationMsTotal` is the workload metric the collapsed row
// shows — see closure note: cycles (ms summed) over the buffered slice
// is unit-stable and directly answers "what is this thread doing right
// now," whereas % active conflates "no tasks scheduled" with "tasks
// finished fast" without a wall-clock window we trust at this layer.
const threadsView = computed(() =>
  taskGraphStore.threads.map((t) => {
    const valid = (t.reports || []).filter(
      (r) => r.startTime && r.finishTime && BigInt(r.finishTime) >= BigInt(r.startTime),
    )
    const sortedRecent = [...valid]
      .sort((a, b) => Number(BigInt(b.startTime) - BigInt(a.startTime)))
      .slice(0, 12)
      .map((r) => ({
        name: r.name,
        durationMs: usToMs(BigInt(r.finishTime) - BigInt(r.startTime)),
      }))
    let totalUs = 0n
    for (const r of valid) totalUs += BigInt(r.finishTime) - BigInt(r.startTime)
    return {
      index: t.index,
      sampleCount: valid.length,
      reports: sortedRecent,
      durationMsTotal: usToMs(totalUs),
    }
  }),
)

// Scaling: the workload sparkline scales against the busiest thread so
// idle threads render visibly empty rather than full-width.
const longestTotalMs = computed(() => {
  let max = 0
  for (const t of threadsView.value) if (t.durationMsTotal > max) max = t.durationMsTotal
  return Math.max(max, 1)
})

const longestReportMs = computed(() => {
  let max = 0
  for (const t of threadsView.value)
    for (const r of t.reports) if (r.durationMs > max) max = r.durationMs
  return Math.max(max, 1)
})

const isExpanded = (index) => uiStore.isTaskThreadExpanded(index)
const toggleThread = (index) => uiStore.toggleTaskThreadExpanded(index)
</script>

<template>
  <div class="task-debugger" data-test="task-debugger-panel">
    <div v-if="!connectionStore.isConnected" class="empty-container">
      <n-empty description="Engine offline" size="small" />
    </div>

    <template v-else>
      <div class="toolbar">
        <n-space align="center" :size="12">
          <n-text depth="3" class="section-label">Thread workload</n-text>
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
        <div class="thread-list" data-test="task-thread-list">
          <template v-for="thread in threadsView" :key="thread.index">
            <button
              type="button"
              class="thread-row"
              :class="{ 'is-expanded': isExpanded(thread.index) }"
              :data-test="`task-thread-row-${thread.index}`"
              :data-thread-index="thread.index"
              :data-expanded="isExpanded(thread.index) ? 'true' : 'false'"
              :aria-expanded="isExpanded(thread.index)"
              @click="toggleThread(thread.index)"
            >
              <span class="chevron" aria-hidden="true">{{ isExpanded(thread.index) ? '▾' : '▸' }}</span>
              <span class="thread-label">Thread {{ thread.index }}</span>
              <div class="bar-track">
                <div
                  class="bar-fill"
                  :style="{ width: ((thread.durationMsTotal / longestTotalMs) * 100).toFixed(1) + '%' }"
                />
              </div>
              <span class="thread-total">{{ thread.durationMsTotal.toFixed(2) }} ms</span>
            </button>

            <div
              v-if="isExpanded(thread.index)"
              class="thread-detail"
              :data-test="`task-thread-detail-${thread.index}`"
            >
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
                      :style="{ width: ((report.durationMs / longestReportMs) * 100).toFixed(1) + '%' }"
                    />
                  </div>
                  <span class="report-duration">{{ report.durationMs.toFixed(2) }} ms</span>
                </div>
              </div>
            </div>
          </template>
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

.thread-list {
  display: flex;
  flex-direction: column;
}

.thread-row {
  display: grid;
  grid-template-columns: 14px 80px 1fr 72px;
  gap: 8px;
  align-items: center;
  height: 24px;
  padding: 0 12px;
  background: transparent;
  border: none;
  border-bottom: 1px solid var(--ctp-surface0);
  color: inherit;
  font: inherit;
  font-size: 12px;
  text-align: left;
  cursor: pointer;
  user-select: none;
}

.thread-row:hover {
  background: var(--ctp-surface0);
}

.thread-row.is-expanded {
  background: var(--ctp-surface0);
}

.chevron {
  font-size: 10px;
  color: var(--ctp-subtext0);
  width: 14px;
  text-align: center;
}

.thread-label {
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.thread-total {
  text-align: right;
  font-variant-numeric: tabular-nums;
  color: var(--ctp-subtext0);
}

.thread-detail {
  padding: 6px 12px 10px 32px;
  border-bottom: 1px solid var(--ctp-surface0);
  background: var(--ctp-mantle);
}

.report-list {
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
  background: var(--ctp-surface1);
  border-radius: 3px;
  overflow: hidden;
}

.thread-row .bar-track {
  height: 5px;
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
