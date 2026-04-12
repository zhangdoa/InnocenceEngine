import { reactive } from 'vue'

export const uiStore = reactive({
  themeFlavor: 'macchiato', // Default flavor

  setTheme(flavor) {
    if (['latte', 'frappe', 'macchiato', 'mocha'].includes(flavor)) {
      this.themeFlavor = flavor;
    }
  }
})
