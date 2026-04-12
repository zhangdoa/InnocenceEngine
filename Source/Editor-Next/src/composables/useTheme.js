import { computed, watch, onBeforeMount } from 'vue'
import { darkTheme, lightTheme } from 'naive-ui'
import { palette } from '../palette'
import { uiStore } from '../store/uiStore'

export function useTheme() {
  const activeTheme = computed(() => uiStore.themeFlavor === 'latte' ? lightTheme : darkTheme)
  const currentPalette = computed(() => palette[uiStore.themeFlavor])

  /**
   * Catppuccin Theme Overrides for Naive UI
   * Centrally managed to ensure contrast and professional aesthetic.
   */
  const themeOverrides = computed(() => ({
    common: {
      primaryColor: currentPalette.value.blue,
      primaryColorHover: currentPalette.value.sky,
      primaryColorPressed: currentPalette.value.sapphire,
      bodyColor: currentPalette.value.base,
      cardColor: currentPalette.value.surface0,
      modalColor: currentPalette.value.mantle,
      popoverColor: currentPalette.value.mantle,
      textColorBase: currentPalette.value.text,
      textColor1: currentPalette.value.text,
      textColor2: currentPalette.value.subtext1,
      textColor3: currentPalette.value.subtext0,
      dividerColor: currentPalette.value.surface1,
      borderColor: currentPalette.value.surface1,
    },
    Menu: {
      itemTextColor: currentPalette.value.text,
      itemIconColor: currentPalette.value.text,
      itemTextColorActive: currentPalette.value.blue,
      itemIconColorActive: currentPalette.value.blue,
    },
    Input: {
      color: currentPalette.value.mantle,
      textColor: currentPalette.value.text,
      border: `1px solid ${currentPalette.value.surface1}`,
    },
    Tag: {
      textColorSuccess: '#11111b',
      textColorError: '#11111b',
      textColorInfo: '#11111b',
      textColorWarning: '#11111b',
    },
    Collapse: {
      titleTextColor: currentPalette.value.text,
    },
    List: {
      textColor: currentPalette.value.text,
    }
  }))

  const updateCssVariables = (flavor) => {
    const p = palette[flavor];
    const root = document.documentElement;
    Object.keys(p).forEach(key => {
      root.style.setProperty(`--ctp-${key}`, p[key]);
    });
  }

  onBeforeMount(() => {
    updateCssVariables(uiStore.themeFlavor);
  });

  watch(() => uiStore.themeFlavor, (newFlavor) => {
    updateCssVariables(newFlavor);
  });

  return {
    activeTheme,
    themeOverrides,
    currentPalette
  }
}
