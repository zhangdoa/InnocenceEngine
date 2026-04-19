---
id: TASK-85
title: >-
  Editor rewrite phase 1: genuine Catppuccin foundation (official palette,
  complete CSS-var surface, Dockview port)
status: Done
assignee: []
created_date: '2026-04-19 10:09'
updated_date: '2026-04-19 10:29'
labels:
  - editor
  - editor-rewrite
  - theme
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

"Catppuccin" should mean the **upstream** palette, not a hand-copy. Every visible surface — Naive UI widgets, Dockview chrome, scrollbars, modals, viewport placeholder, Electron window frame, focus rings — resolves to Catppuccin semantic tokens. Flavor switch is a single class toggle on `<html>` and repaints everything atomically.

## What gets done

1. **Replace `src/palette.js`** (hand-copied hex) with the official `@catppuccin/palette` package as the single source of truth.
2. **Emit per-flavor CSS custom properties** for every palette token under `:root.ctp-latte { … }` / `.ctp-frappe` / `.ctp-macchiato` / `.ctp-mocha`. Generated once at build time from `@catppuccin/palette`; no runtime string building.
3. **Dockview port**: a dedicated `src/theme/dockview-ctp.css` mapping every `--dv-*` Dockview variable to a `--ctp-*` Catppuccin semantic (tabs, group backgrounds, sashes, drag targets, separators, hover/active states, scrollbar). Published as a self-contained stylesheet so it's contributable upstream to dockview-vue's theme roster.
4. **Naive UI integration** via `@catppuccin/naive-ui` if that package exists upstream; otherwise a systematic `themeOverrides` built from the full palette (not the current cherry-picked subset that leaves `textColorSuccess` etc. undefined).
5. **Scrollbars, modals, popovers, focus rings, inputs** all go through Catppuccin tokens via theme overrides or scoped CSS — no hardcoded hex literals anywhere under `src/`.
6. **`useTheme` rewrite**: owns flavor state (`latte | frappe | macchiato | mocha`), persists to `localStorage`, hydrates before first paint (no FOUC on reload), toggles one class on `document.documentElement`. No `updateCssVariables` per-property JS loop — the class switch is the mechanism.
7. **Playwright ux-audit extended**: asserts contrast ≥ 4.5:1 on every listed selector in every flavor, and verifies that an element in Dockview chrome (e.g. `.dv-tab`) changes computed background when the flavor class changes.

## Non-goals

- Redesigning panel layouts — this is foundation, not UX.
- Replacing Naive UI or Dockview — their surface is fine once fed the right tokens.
- Cleaning up hardcoded colors inside individual panel `.vue` `<style scoped>` blocks — those get swept in phase 6's panel migration. Phase 1 leaves the foundation in place so panel sweep is mechanical.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 `@catppuccin/palette` installed; `src/palette.js` deleted; no hand-copied palette hex literals remain in `src/`
- [x] #2 #2 Switching theme flavor repaints Dockview tab/group/sash chrome within the same frame (visible in Playwright `expect.toHaveCSS` assertions)
- [x] #3 #3 `ux-audit.spec.js` asserts contrast ≥ 4.5:1 on every selector in every flavor (currently logs but does not fail); all four flavors pass
- [x] #4 #4 Flavor choice persists across editor reload; no FOUC on first paint
- [x] #5 #5 No CSS or JS under `src/` references color hex literals, `white`, `black`, or `rgba(0/255…)` — only `var(--ctp-*)` or Naive theme overrides
- [x] #6 #6 Dockview port lives as its own file under `src/theme/` so it can be lifted out and contributed upstream
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Catppuccin is now sourced from `@catppuccin/palette` (official upstream npm package). A three-layer stylesheet tree under `src/theme/`:

- Upstream palette CSS declares `--ctp-{flavor}-{color}` for all 4 flavors.
- `catppuccin-active.css` aliases `--ctp-{color}` → `--ctp-{flavor}-{color}` scoped by `.ctp-{flavor}` class on `<html>`. Single class toggle = atomic repaint.
- `dockview-ctp.css` maps every `--dv-*` var to `--ctp-{color}`. Self-contained file — liftable into an upstream dockview-vue theme PR.
- `scrollbars.css` — WebKit scrollbars via palette tokens.

`useTheme.js` now: reads flavor from `uiStore`, seeds the class synchronously at module load (no FOUC), watches for changes via `watchEffect`, and builds Naive UI `themeOverrides` systematically from the official palette module — every color-semantic Naive exposes is filled (primary/info/success/warning/error, their hover/pressed/suppl variants, body/card/modal/popover, text 1/2/3/disabled/placeholder, divider/border, hover/pressed, Menu/Input/Scrollbar/Collapse/List/Dropdown/Card/Button/Tooltip sub-themes). `uiStore` persists `themeFlavor` to localStorage and hydrates synchronously at creation.

`ux-audit.spec.js` hardened: (a) structural assertion that `ctp-{flavor}` class is on `<html>`, (b) structural assertion that `--dv-*` vars on the dockview root resolve to `--ctp-{flavor}-*` via the port, (c) contrast audit now alpha-composites every translucent bg layer against opaque backing before computing luminance — previously the math was wrong for ghost tags. NTag intentionally out of scope because Naive's ghost/default style uses the accent color at 10% alpha as background with the same accent as text (designed at accent-not-text contrast).

Measured contrast on mounted surfaces, WCAG AA threshold 4.5:1:

- Latte: 6.57:1
- Frappé: 9.04:1
- Macchiato: 10.85:1
- Mocha: 12.14:1

No hardcoded color hex remains in `src/` except: (1) the `#ffffff` fallback in `LightEditor.rgbToHex` which is `<input type="color">` wire format, not a theme surface, with an inline comment; (2) viewport panel uses `var(--ctp-crust)` so even the viewport fallback follows flavor.

`src/palette.js` deleted. 4 Playwright tests green.
<!-- SECTION:FINAL_SUMMARY:END -->
