import { computed, watchEffect } from 'vue'
import { darkTheme, lightTheme } from 'naive-ui'
import { flavors as ctpFlavors } from '@catppuccin/palette'
import { uiStore, FLAVORS } from '../store/uiStore'

/**
 * Catppuccin theme integration.
 *
 * - Flavor state lives in `uiStore.themeFlavor` (persisted to localStorage
 *   by the store itself).
 * - CSS custom properties come from `src/theme/*.css` — the active-flavor
 *   layer aliases `--ctp-{color}` to the chosen flavor's palette entry.
 *   This composable just toggles one class on `<html>`; the class switch
 *   repaints everything.
 * - Naive UI can't consume CSS vars in its themeOverrides (it snapshots
 *   concrete colors at prop-change time), so we build overrides from the
 *   official `@catppuccin/palette` JS export.
 *
 * Seed the class synchronously at module load so the first paint is
 * flavor-correct. Without this, the DOM renders unstyled for one frame
 * before the watchEffect runs.
 */
if (typeof document !== 'undefined') {
  document.documentElement.classList.add(`ctp-${uiStore.themeFlavor}`)
}

export function useTheme() {
  const currentPalette = computed(
    () => ctpFlavors[uiStore.themeFlavor].colors,
  )

  const activeTheme = computed(() =>
    uiStore.themeFlavor === 'latte' ? lightTheme : darkTheme,
  )

  const themeOverrides = computed(() => {
    const p = currentPalette.value
    const c = (k) => p[k].hex
    return {
      common: {
        primaryColor:        c('blue'),
        primaryColorHover:   c('sky'),
        primaryColorPressed: c('sapphire'),
        primaryColorSuppl:   c('lavender'),

        infoColor:           c('sapphire'),
        infoColorHover:      c('sky'),
        infoColorPressed:    c('blue'),
        infoColorSuppl:      c('sky'),

        successColor:        c('green'),
        successColorHover:   c('teal'),
        successColorPressed: c('green'),
        successColorSuppl:   c('teal'),

        warningColor:        c('yellow'),
        warningColorHover:   c('peach'),
        warningColorPressed: c('yellow'),
        warningColorSuppl:   c('peach'),

        errorColor:          c('red'),
        errorColorHover:     c('maroon'),
        errorColorPressed:   c('red'),
        errorColorSuppl:     c('maroon'),

        bodyColor:    c('base'),
        cardColor:    c('surface0'),
        modalColor:   c('mantle'),
        popoverColor: c('mantle'),

        textColorBase: c('text'),
        textColor1:    c('text'),
        textColor2:    c('subtext1'),
        textColor3:    c('subtext0'),
        textColorDisabled: c('overlay0'),
        placeholderColor:  c('subtext0'),

        dividerColor: c('surface1'),
        borderColor:  c('surface1'),

        hoverColor:    c('surface0'),
        pressedColor:  c('surface1'),

        invertedColor: c('crust'),
      },
      Menu: {
        itemTextColor:           c('text'),
        itemIconColor:           c('text'),
        itemTextColorActive:     c('blue'),
        itemIconColorActive:     c('blue'),
        itemTextColorHover:      c('blue'),
        itemIconColorHover:      c('blue'),
        itemColorActive:         c('surface0'),
        itemColorActiveHover:    c('surface0'),
      },
      Input: {
        color:       c('mantle'),
        colorFocus:  c('mantle'),
        textColor:   c('text'),
        border:      `1px solid ${c('surface1')}`,
        borderHover: `1px solid ${c('surface2')}`,
        borderFocus: `1px solid ${c('blue')}`,
        placeholderColor: c('subtext0'),
      },
      InputNumber: {
        iconColor: c('subtext1'),
      },
      Scrollbar: {
        color:      c('surface1'),
        colorHover: c('surface2'),
      },
      Collapse: {
        titleTextColor: c('text'),
      },
      List: {
        textColor: c('text'),
        color:     'transparent',
      },
      Dropdown: {
        color:           c('mantle'),
        optionColorHover: c('surface0'),
        optionTextColor:  c('text'),
      },
      Card: {
        color:       c('surface0'),
        colorModal:  c('mantle'),
        borderColor: c('surface1'),
        titleTextColor: c('text'),
      },
      Button: {
        textColorHover:   c('blue'),
        textColorPressed: c('sapphire'),
      },
      Tooltip: {
        color:     c('mantle'),
        textColor: c('text'),
      },
    }
  })

  watchEffect(() => {
    if (typeof document === 'undefined') return
    const root = document.documentElement
    for (const f of FLAVORS) root.classList.remove(`ctp-${f}`)
    root.classList.add(`ctp-${uiStore.themeFlavor}`)
  })

  return { activeTheme, themeOverrides, currentPalette }
}
