import { reactive, watch } from 'vue'

const STORAGE_KEY = 'editor-next.themeFlavor'
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

export const uiStore = reactive({
  themeFlavor: readPersistedFlavor(),

  setTheme(flavor) {
    if (FLAVORS.includes(flavor)) this.themeFlavor = flavor
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

export { FLAVORS }
