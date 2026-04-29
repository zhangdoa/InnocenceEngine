# Discipline: visual-validation

A single static capture is never sufficient evidence for a change that affects rendered output. A visual claim requires a frame sequence (to see temporal behaviour), multiple camera angles, and a reference render where applicable.

## How

Visual evidence supporting a CL must include:

- A frame sequence long enough to see temporal behaviour (flicker, boil, accumulation artefacts).
- Multiple camera angles — what's stable from one view may mask a defect visible from another.
- A reference render for ground-truth comparison where applicable.

Archive captures under `Build/captures/` with labels tied to the work. The archive is how the project proves quality goes up over time, not just sideways.

### A/B toggle pattern for shader-feature validation

When a shader feature's contribution is subtle in the only test scene currently available (e.g. point-shadow occlusion delta on GISponza is ~3% mean luminance because GI + sun dominate the budget), a `#define`-gated bypass in the consuming HLSL is the bridge between "implementation correct" and "dedicated test scene authored":

- The toggle disables only the new contribution; everything else stays identical.
- Capture before-frame (toggle on, feature bypassed) and after-frame (toggle off, feature live) at the same camera/animation state.
- Compare numerically (mean luminance per channel) and visually. Numeric direction must match the physical expectation (added shadow → darker, added GI bounce → brighter, etc.).
- An A/B toggle is necessary-but-not-sufficient evidence. It rules out "the feature did nothing"; it does not validate "the feature is correct in all configurations." A dedicated test scene + RenderDoc capture remains the closure target — the toggle is the deferred-quality bridge, not the substitute.

## Recorded incident

`DEBUG_POINT_SHADOW_BYPASS` in `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` (TASK-148, commit `71817f3a`, 2026-04-26). The point-shadow contribution on GISponza was ~3% mean-luminance delta — small enough that "before/after look the same" was a real risk. The toggle made the contribution measurable on existing content while a dedicated test scene was deferred.

## Cross-references

- `peer-review-required.md` — the UNVERIFIED review tier exists for visual ACs that pass numerically but cannot actually be exercised by the test infra (auto-capture culling, headless harness limits). Reviewer must promote test-infra-blocked visual ACs to UNVERIFIED, not ADVISORY.
- `perf-measurement-frame-budget.md` — *visual-correctness verification* is explicitly out of scope for the perf-measurement N-picking rule; this discipline owns frame counts for visual claims.
