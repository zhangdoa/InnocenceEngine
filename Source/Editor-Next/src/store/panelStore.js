import { reactive, shallowRef } from 'vue'

// Single source of truth for what panels exist and which are currently
// open. AppLayout registers descriptors here at mount; EditorHeader's
// Window menu reads the list and dispatches toggles. dockview-vue's
// addPanel / removePanel handles the actual layout mutation.
//
// Layout persistence (panel positions surviving editor reload) is a
// follow-up — for now, closing and re-opening returns the panel to its
// declared default position. That's better than the prior state where
// closing a panel by accident lost it until restart.

const dockviewApi = shallowRef(null)

export const panelStore = reactive({
  // [{ id, component, title, position, visible }]
  panels: [],

  setApi(api) {
    dockviewApi.value = api
    // First wire-up: the descriptors registered before the API showed up
    // get added now so the initial layout matches what AppLayout asked for.
    this.panels.forEach((p) => {
      if (p.visible && !dockviewApi.value.getPanel(p.id)) this._addToDock(p)
    })
  },

  register(descriptor) {
    if (this.panels.find((p) => p.id === descriptor.id)) return
    const entry = { ...descriptor, visible: true }
    this.panels.push(entry)
    if (dockviewApi.value && !dockviewApi.value.getPanel(entry.id)) this._addToDock(entry)
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

  _addToDock(p) {
    // Re-resolve the position's referencePanel by id at add-time. Stored
    // position descriptors that name a sibling panel can become stale if
    // that sibling is closed; fall back to a free-floating placement so
    // dockview doesn't error on a missing reference.
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
})
