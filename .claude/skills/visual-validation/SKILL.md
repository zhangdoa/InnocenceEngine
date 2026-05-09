---
name: visual-validation
description: Use on every CL touching rendered output (passes, shaders, cache, denoise, composition, tone-map, post-process, lighting, materials). Four-layer validation; numeric metrics never substitute for agent visual Read.
---

# Skill: visual-validation

A single static capture is never sufficient evidence for a change that affects rendered output. Numeric metrics (stddev, mean luminance, MAE, per-pixel deltas) are structurally blind to spatial artifacts — they cannot distinguish a clean low-noise frame from one covered in a stable cell pattern. Numeric closure without an agent visual `Read` is closure on a proxy.

Every CL touching rendering output (passes, shaders, cache, denoise, composition, tone-map, post-process, lighting, materials) walks all four layers in order.

## Layer 1 — Agent visual `Read` (mandatory)

Before declaring closure on any rendering-output CL, the implementing agent `Read`s two PNGs (the high-SPP self-reference from layer 3 and the candidate) and writes this block in the closure record:

```
Visual Read assessment
- What I see in reference: <descriptive: lighting, GI bounce, materials, structure, artifacts>
- What I see in current:   <same checklist for the candidate>
- Differences:             <enumerated spatial and temporal differences>
- Verdict:                 improvement | regression | uncertain
```

Block is descriptive, not numeric. "p50 dropped 5.5×" is layer 2/3; layer 1 is "the floor has a checkerboard of color squares not present in the reference." Decline / unable to `Read` → verdict `uncertain` → layer 4 fires.

## Layer 2 — Frame sequence + multi-angle

- Frame sequence long enough to see temporal behaviour.
- Multiple camera angles.
- Reference render where applicable (layer 3).

Captures archived under `Build/captures/`.

## Layer 3 — High-SPP self-reference

For path-tracer denoising, radiance-cache filtering, accumulation, or any "approximate the converged result more cheaply" pass: the reference is the same engine, same camera, same scene, same frame budget — minus the mechanism under test. Not CPU PT, not a saved baseline image.

**Capture protocol.** Reference: mechanism OFF, high SPP, deterministic dump frame from `-dump_frames`. Candidate: mechanism ON, low SPP, **same** camera / scene / accumulation / dump frame. Capture flags in `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`.

**Three-scene minimum.** Every rendering-output CL captures unit-test scene + GI test box + GI Sponza. Different scenes expose different failure modes. Missing any scene = layer-2-incompleteness BLOCKED finding. Layer-1 assessment applies per scene per angle per sampled frame, not collapsed.

**Toggle pattern (3b).** When a feature's contribution is subtle, a `#define`-gated bypass in the consuming HLSL bridges before/after captures. Numeric direction must match physical expectation. A/B toggle is necessary-but-not-sufficient (rules out "feature did nothing"; doesn't validate correctness).

Each toggle has either a documented retire criterion (transient, measurable: "remove when on-path beats off-path on `<metric>` by `<margin>`") or a permanence rationale (durable seam: denoiser variants, integrator strategies, reference-path preservation). Compile-time (`#define` + `if constexpr`, bit-identical when off) vs runtime (`DevToggleRegistry`, live flipping) is a separate axis.

**PT-as-rasterizer-reference (3c).** Only fair on dimensions both pipelines compute. Enumerate omitted features (unshadowed light types, transmission/refraction, multi-bounce, post-effects, AA modes) and either disable them in PT or annotate the comparison.

**MAE-vs-CPU-PT is not the reference for PT denoising work (3d).** `TestGIScene.ps1` MAE check is a sanity floor against gross breakage of the rasterized GI path, not a quality bar for denoising / cache filtering. CPU PT is pre-historical; MAE averages spatially and cannot see cell-pattern artifacts; rasterizer-vs-PT is the wrong axis for "did denoising preserve quality."

## Layer 4 — User-eye fallback (the ratchet)

User is the final visual arbiter. Sign-off enters the closure record. Layer 4 fires when **any** of:

- Layer-1 verdict is `uncertain`.
- CL touches **composition**, **denoise**, **cache**, **tone-map**, **post-process**, or any **first-time-scene × first-time-feature** combination.
- Layer-1 verdict **disagrees with numeric metrics**.

On trigger: pause closure, surface block + frames + disagreement to the user, wait for sign-off.

## Cross-references

- `peer-review-required` — UNVERIFIED tier, reviewer-visual-inspection mandate, `Reviewed-Visually:` footer, `visual-review` gate.
- `perf-frame-budget` — visual verification out of scope for N-picking.
- `regression-build-chain`, `dispatch-briefs` — stop-the-line on accumulating carry-forwards.
