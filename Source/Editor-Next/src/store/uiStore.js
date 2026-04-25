import { reactive, watch } from 'vue'

const STORAGE_KEY = 'editor-next.themeFlavor'
const TASK_EXPAND_KEY = 'editor-next.taskDebugger.expanded'
const FLAVORS = ['latte', 'frappe', 'macchiato', 'mocha']

function readPersistedFlavor() {
  try {
    const stored = localStorage.getItem(STORAGE_KEY)
    if (stored && FLAVORS.includes(stored)) return stored
  } catch {
    // localStorage inaccessible (restricted context); fall through to default
  }
  return 'macchiato'
}

function readPersistedTaskExpand() {
  try {
    const stored = localStorage.getItem(TASK_EXPAND_KEY)
    if (!stored) return {}
    const parsed = JSON.parse(stored)
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) return {}
    // Coerce to {[stringIndex]: boolean}; ignore malformed entries rather than
    // throwing — a corrupt entry shouldn't take down the whole panel.
    const out = {}
    for (const [k, v] of Object.entries(parsed)) {
      if (typeof v === 'boolean') out[String(k)] = v
    }
    return out
  } catch {
    return {}
  }
}

export const uiStore = reactive({
  themeFlavor: readPersistedFlavor(),
  // Per-thread expand/collapse for TaskDebuggerPanel. Keyed by stringified
  // thread index. Absent key → collapsed (the default per TASK-92).
  taskDebuggerExpanded: readPersistedTaskExpand(),

  setTheme(flavor) {
    if (FLAVORS.includes(flavor)) this.themeFlavor = flavor
  },

  isTaskThreadExpanded(index) {
    return !!this.taskDebuggerExpanded[String(index)]
  },

  setTaskThreadExpanded(index, expanded) {
    const key = String(index)
    if (expanded) this.taskDebuggerExpanded[key] = true
    else delete this.taskDebuggerExpanded[key]
  },

  toggleTaskThreadExpanded(index) {
    this.setTaskThreadExpanded(index, !this.isTaskThreadExpanded(index))
  },
})

// Persist on change. Hydrating the flavor before the store is created (above)
// means the first paint already reflects the user's last choice — no FOUC.
watch(
  () => uiStore.themeFlavor,
  (flavor) => {
    try { localStorage.setItem(STORAGE_KEY, flavor) } catch { /* ignore */ }
  },
)

watch(
  () => uiStore.taskDebuggerExpanded,
  (next) => {
    try { localStorage.setItem(TASK_EXPAND_KEY, JSON.stringify(next)) } catch { /* ignore */ }
  },
  { deep: true },
)

export { FLAVORS }
