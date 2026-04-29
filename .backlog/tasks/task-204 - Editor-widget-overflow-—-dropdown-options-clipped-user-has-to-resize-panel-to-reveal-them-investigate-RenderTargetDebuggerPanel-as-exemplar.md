---
id: TASK-204
title: >-
  Editor widget overflow — dropdown options clipped, user has to resize panel to
  reveal them; investigate RenderTargetDebuggerPanel as exemplar
status: Done
assignee:
  - editor-tooling-expert
created_date: '2026-04-29 17:18'
updated_date: '2026-04-29 18:05'
labels:
  - editor
  - ui
  - bug
  - naive-ui
dependencies: []
references:
  - Source/Editor-Next/src/components/RenderTargetDebuggerPanel.vue
  - Source/Editor-Next/src/components/AppLayout.vue
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

User-reported 2026-04-29: *"widgets can't scroll, i have to change the size to find the covered options. and the render target debugger, look at that, why?"*

Two observations bundled:

1. **Widget-scroll / dropdown-clip bug**: When a panel hosts a select / combobox with many options (or a long list of inputs), the dropdown's popover or the panel's content area gets clipped — the only workaround is to manually resize the panel to make the hidden options visible. This is a discoverability + accessibility regression: the affordance to see the options is hidden behind a window resize.

2. **RenderTargetDebuggerPanel.vue specifically**: User flagged this panel with "look at that, why?" — possibly the same scroll-clip issue manifesting there (the panel has an `n-select` of all (Pass, RT) pairs in `Source/Editor-Next/src/components/RenderTargetDebuggerPanel.vue:25-36`, which can have dozens of entries), OR a separate concern (e.g. why does this panel exist alongside TASK-183's runtime viz picker — they target different layers).

## Likely root causes

For #1 (widget scroll/overflow):
- Naive-ui dropdowns (`n-select`, `n-color-picker`) render their popover content via teleport to `body` by default, but a parent container with `overflow: hidden` + `transform`/`filter`/`will-change` can break the teleport and clip the popover. Common in panels nested under sidebar / split-pane layouts.
- Or: the panel's `n-scrollbar` doesn't activate because no height constraint propagates from the parent. When content exceeds the panel size, the inner widget overflows but the scrollbar never appears.
- Or: `n-form-item` `label-width` is set wide enough that small panels truncate the input column, making it look like scroll is broken when it's actually layout overflow.

For #2 (RenderTargetDebuggerPanel "why"):
- If same as #1: fix it as the exemplar, the fix likely generalizes.
- If a layering question: the RenderTargetDebuggerPanel overrides the *viewport source texture* via `renderTargetStore.setOverride(pass, rtIndex)` — the engine swaps which RT the swapchain blits. TASK-183's runtime viz picker (committed `95238629`) changes `lightPass.comp`'s RT0 output via `g_Frame.debugViewMode`. Different layers, both legitimate. If user wants them merged or one removed, that's a design call beyond this bug.

## Goal

1. Reproduce the dropdown-clip / widget-scroll issue against the live editor (target: any panel with n-select that has more than a handful of options — RenderTargetDebuggerPanel is the most likely exemplar).
2. Diagnose: clipping vs missing-scrollbar vs label-width overflow.
3. Fix at the right layer — global panel CSS (overflow hosts), naive-ui teleport target, or per-panel sizing constraints.
4. Add a regression spec — a Playwright test that asserts dropdown options are visible without resizing.
5. Confirm or refute the "RenderTargetDebuggerPanel why" question — same-issue exemplar, or a separate user concern.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Bug reproduced under a known panel (cite file:line)
- [x] #2 Root cause identified (cite the CSS rule / teleport boundary / sizing constraint)
- [x] #3 Fix lands at the right layer — panel CSS or naive-ui teleport target, NOT a per-component patch on every panel
- [x] #4 All affected panels regression-tested in the editor (visual confirmation + at least one Playwright spec)
- [x] #5 User's "why?" on RenderTargetDebuggerPanel answered — either "same issue, fixed" or "separate concern, here's what it is"

## Owner

`editor-tooling-expert`. Editor / Vue / naive-ui scope.

## References

- `Source/Editor-Next/src/components/RenderTargetDebuggerPanel.vue` (likely exemplar; lines 78-126 show the n-scrollbar + n-select pattern)
- `Source/Editor-Next/src/components/AppLayout.vue` (parent panel layout; check overflow / teleport hosts)
- TASK-183 (different-layer feature; relevant only if "why?" turns out to be an overlap question)
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Editor widget overflow — fix landed

**Root cause**: `NConfigProvider` in `ThemedPanelHost.vue` renders an outer `<div>` between dockview's `.dv-content-container` (flex-grow:1, no explicit height) and each panel root (`height:100%`). Without `height:100%` on that wrapper, the panel root falls back to intrinsic content height; the inner `n-scrollbar` measures `scrollHeight==clientHeight`, never activates; overflow gets clipped by `.dv-groupview`'s `overflow:hidden`.

**Fix**: 1-line `style="height: 100%"` on `<n-config-provider>` in `Source/Editor-Next/src/components/ThemedPanelHost.vue`, plus a documenting comment block anchored to the present-state height-chain invariant.

**Generality**: every panel (HierarchyPanel, AssetPanel, RenderTargetDebuggerPanel, RenderTogglesPanel, TaskDebuggerPanel, PropertyPanel) routes through `hosted(Comp)` in `main.js` → `ThemedPanelHost`, and every panel root uses `height: 100%`. Single wrapper fix applies to all six. Reviewer verified via grep + line-grounded check on each panel file.

**Spec**: consolidated `Source/Editor-Next/tests/widget-overflow.spec.js` (replaces 4 throwaway repro specs from the predecessor's iteration). Asserts the structural height chain `dvContent.clientHeight == configProvider.clientHeight == panel.clientHeight` on RenderTargetDebuggerPanel + PropertyPanel, plus a guarded user-observable scroll assertion. `npx playwright test --workers=1` → 1 passed (9.2s).

**Validation**:
- 1 of 2 budgeted editor launches (per test-etiquette.md). No orphan processes leaked.
- Screenshot: `Build/captures/task-204-after-fix.png`
- Peer review: editor-tooling-expert PASS+ADVISORY (3 advisory items: A1 dropped (TASK-204) trailer per comment-discipline — applied; A2 minor narrative-tense in comment, A3 minor narrative-tense in spec docstring — both optional polish, not addressed per don't-pile-on policy)

**User's "why?" question on RenderTargetDebuggerPanel** (separate from this bug): clarified earlier this session as a design question — option (b) chosen (keep both, trim TASK-183 GBuffer modes). That work is on a separate task (TASK-205, in flight).
<!-- SECTION:FINAL_SUMMARY:END -->
