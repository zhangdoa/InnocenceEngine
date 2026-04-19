import { reactive, shallowRef } from 'vue'

// Single source of truth for what panels exist and which are currently
// open. AppLayout registers descriptors here at mount; EditorHeader's
// Window menu reads the list and dispatches toggles. Layout is persisted
// to localStorage on every dockview-driven change and restored on next
// editor launch.

const LAYOUT_STORAGE_KEY = 'inno-editor-layout-v1'
const PERSIST_DEBOUNCE_MS = 250

const dockviewApi = shallowRef(null)
let persistTimer = null

const debouncedPersist = () => {
  if (persistTimer) clearTimeout(persistTimer)
  persistTimer = setTimeout(() => {
    persistTimer = null
    try {
      const state = dockviewApi.value?.toJSON()
      if (state) localStorage.setItem(LAYOUT_STORAGE_KEY, JSON.stringify(state))
    } catch (err) {
      console.warn('panelStore: layout persist failed', err)
    }
  }, PERSIST_DEBOUNCE_MS)
}

const loadStoredLayout = () => {
  try {
    const raw = localStorage.getItem(LAYOUT_STORAGE_KEY)
    return raw ? JSON.parse(raw) : null
  } catch (err) {
    console.warn('panelStore: layout load failed; starting fresh', err)
    return null
  }
}

export const panelStore = reactive({
  // [{ id, component, title, position, visible }]
  panels: [],

  setApi(api) {
    dockviewApi.value = api

    const stored = loadStoredLayout()
    if (stored) {
      try {
        api.fromJSON(stored)
        this._syncVisibilityFromDock()
      } catch (err) {
        console.warn('panelStore: fromJSON rejected stored layout; falling back to defaults', err)
        this._addAllRegistered()
      }
    } else {
      this._addAllRegistered()
    }

    // Subscribe to dockview's layout-change events so subsequent mutations
    // (drag, dock, float, close) get persisted.
    if (api.onDidLayoutChange) api.onDidLayoutChange(debouncedPersist)
  },

  register(descriptor) {
    if (this.panels.find((p) => p.id === descriptor.id)) return
    const isAlreadyOpen = !!dockviewApi.value?.getPanel(descriptor.id)
    const entry = { ...descriptor, visible: isAlreadyOpen || !dockviewApi.value }
    this.panels.push(entry)
    if (dockviewApi.value && !isAlreadyOpen) this._addToDock(entry)
  },

  toggle(id) {
    const p = this.panels.find((d) => d.id === id)
    if (!p || !dockviewApi.value) return
    if (p.visible) {
      const handle = dockviewApi.value.getPanel(id)
      if (handle) handle.api.close()
      p.visible = false
    } else {
      this._addToDock(p)
      p.visible = true
    }
  },

  isVisible(id) {
    return !!this.panels.find((d) => d.id === id && d.visible)
  },

  resetLayout() {
    try {
      localStorage.removeItem(LAYOUT_STORAGE_KEY)
    } catch (_) {
      // localStorage might be unavailable in some sandboxed contexts
    }
    const api = dockviewApi.value
    if (!api) return
    // Tear down the whole dockview in one atomic call instead of looping
    // handle.close() (which was async and could race the subsequent
    // re-add, leaving some panels registered from the old layout and
    // skipped in _addAllRegistered). `clear()` removes every panel and
    // group synchronously.
    if (typeof api.clear === 'function') {
      api.clear()
    } else {
      // Fallback for older dockview versions without clear(): close each
      // panel and flip visibility; known to race on some transitions but
      // still better than no reset.
      this.panels.forEach((p) => {
        const handle = api.getPanel(p.id)
        if (handle) handle.api.close()
      })
    }
    this.panels.forEach((p) => { p.visible = false })
    this._addAllRegistered()
  },

  _addAllRegistered() {
    this.panels.forEach((p) => {
      if (!dockviewApi.value.getPanel(p.id)) this._addToDock(p)
      p.visible = true
    })
  },

  _addToDock(p) {
    // Re-resolve the position's referencePanel by id at add-time. A stored
    // descriptor that names a sibling panel can become stale if that
    // sibling is currently closed; fall back to a free-floating placement
    // so dockview does not error on a missing reference.
    const pos = { ...p.position }
    if (pos.referencePanel && typeof pos.referencePanel === 'string') {
      if (!dockviewApi.value.getPanel(pos.referencePanel)) {
        pos.direction = 'within'
        pos.referencePanel = null
      }
    }
    dockviewApi.value.addPanel({
      id: p.id,
      component: p.component,
      title: p.title,
      position: pos,
    })
  },

  _syncVisibilityFromDock() {
    if (!dockviewApi.value) return
    this.panels.forEach((p) => {
      p.visible = !!dockviewApi.value.getPanel(p.id)
    })
  },
})
