import { reactive } from 'vue'
import { request, on } from '../composables/useIpc'
import { connectionStore } from './connectionStore'

/**
 * Mirrors the engine's per-thread task reports. Pull-only for now (LIST_TASKS
 * reply). A streaming TASK_GRAPH_FRAME event is a later addition (phase 6
 * spec in TASK-62's Concurrency / Task Debugger pane).
 */
export const taskGraphStore = reactive({
  threads: [], // [{ index, reports: [{ name, startTime, finishTime }] }]

  reset() {
    this.threads = []
  },

  async refresh() {
    if (!connectionStore.isConnected) return
    const result = await request('LIST_TASKS')
    this.threads = result?.threads ?? []
  },
})

on('engine-connected', ({ connected }) => {
  if (!connected) taskGraphStore.reset()
})

on('TASK_GRAPH_FRAME', (payload) => {
  if (payload?.threads) taskGraphStore.threads = payload.threads
})
