# Discipline: visual-validation

A single static capture is never sufficient evidence for a change that affects rendered output. A visual claim requires a frame sequence (to see temporal behaviour), multiple camera angles, and a reference render where applicable.

Numeric metrics are **structurally blind to spatial artifacts**. Temporal stddev, mean luminance, MAE-vs-baseline, and per-pixel deltas all average across space — they cannot distinguish a clean low-noise frame from a frame covered in a stable cell pattern. Numeric closure without an agent visual `Read` (layer 1 below) is closure on a proxy, not on quality.

## Layered validation

Every CL that touches rendering output (rendering passes, shaders, cache, denoise, composition, tone-map, post-process, lighting, materials) walks all four layers in order. Layer 1 is mandatory. Layers 2–3 apply where the change shape calls for them. Layer 4 is the ratchet, triggered by the conditions listed below.

### Layer 1 — Agent visual `Read` (structured, mandatory)

The implementing agent is a multimodal LLM with a `Read` tool that ingests PNGs. Before declaring closure on any rendering-output CL, the agent MUST `Read` two PNGs — the high-SPP self-reference (layer 3) and the candidate — and write the following structured block into the closure record (Implementation Note on the task and/or commit-message body):

```
Visual Read assessment
- What I see in reference: <descriptive paragraph — lighting character, GI bounce
  contribution, surface materials, spatial structure, presence/absence of artifacts>
- What I see in current:   <same descriptive checklist for the candidate frame>
- Differences:             <enumerated list of spatial and temporal differences>
- Verdict:                 improvement | regression | uncertain
```

If the agent declines or is unable to `Read` a candidate (e.g. capture failed, file missing), the Verdict is automatically `uncertain` and layer 4 fires.

The block is descriptive, not numeric. "p50 dropped 5.5×" is a layer-2/3 observation; layer 1 is "the floor and walls have a checkerboard of color squares that does not appear in the reference." The point of layer 1 is that the agent is *forced to look* and forced to *describe what it sees*, so that spatial artifacts which numeric metrics cannot see become impossible to ship past closure.

### Layer 2 — Frame sequence and multi-angle

Visual evidence supporting a CL must include:

- A frame sequence long enough to see temporal behaviour (flicker, boil, accumulation artefacts).
- Multiple camera angles — what's stable from one view may mask a defect visible from another.
- A reference render for ground-truth comparison where applicable (see layer 3).

Archive captures under `Build/captures/` with labels tied to the work.

### Layer 3 — High-SPP self-reference (the reference for PT denoising / cache work)

For path-tracer denoising, radiance-cache filtering, accumulation, or any pass whose job is "approximate the converged result more cheaply," the **reference is the engine itself, rendering the same scene with the cheap mechanism turned off and convergence forced by raw sample count.** Not CPU PT. Not a saved baseline image. The same engine, same camera, same scene, same frame budget — minus the denoiser / cache / approximation under test.

#### 3a — Capture protocol

- Reference: cache OFF (or denoiser bypassed, or whatever mechanism is under evaluation disabled), high SPP per frame and/or many accumulated frames — whichever the engine's existing capture infra already produces. No new infra. The deterministic dump frame is whichever frame the auto-capture or `-dump_frames` path emits at the end of accumulation.
- Candidate: cache ON (or denoiser ON, or feature live), low SPP, **same** camera, **same** scene, **same** accumulation count, **same** dump frame.
- Capture flags currently shipping (see `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`): `-total_frames N` for the run length, `-dump_frames START-END` for the per-frame PNG sequence used to feed layer 1 and layer 2.

**Three-scene minimum.** Every rendering-output CL captures in all three: unit-test scene, GI test box, GI Sponza. Not "pick one"; not "Sponza is enough"; not "the implementer chooses based on what the change touches." Different scenes expose different failure modes — sharp geometry edges, thin occluders, large flat surfaces, complex BRDFs — and one-scene closure protocols are how visible regressions ship past numeric-green ACs. The dispatch brief names the three scenes explicitly, never "appropriate scenes." Layer-2 visual evidence in the closure record (Implementation Note + commit body) must include captures from all three scenes; missing any scene is a layer-2-incompleteness BLOCKED-tier finding for the reviewer. Layer-1 *Visual Read assessment* applies per scene per angle per sampled frame, not collapsed across scenes.

Recorded incident: the TASK-77.1 phase-1 / phase-1.5 chain shipped a ring-artifact regression that was invisible in the GISponza fixed-camera capture used for closure.

The "no new infra" constraint is load-bearing: the discipline cannot be added on top of work that is itself blocked on a tooling rebuild. Use the toggle / bypass pattern below (3b) to produce the reference from the same binary that produced the candidate.

#### 3b — Reference-via-bypass toggle pattern

When a feature's contribution is subtle in the only test scene available, a `#define`-gated bypass in the consuming HLSL is the bridge between "implementation correct" and "dedicated test scene authored":

- The toggle disables only the new contribution; everything else stays identical.
- Capture before-frame (toggle on, feature bypassed → this is the reference for cache/denoise work, or the *baseline* for additive features) and after-frame (toggle off, feature live → candidate).
- Compare numerically (mean luminance per channel) and visually via layer 1. Numeric direction must match the physical expectation (added shadow → darker, added GI bounce → brighter, denoise / cache → lower variance with preserved spatial structure, etc.).
- An A/B toggle is necessary-but-not-sufficient. It rules out "the feature did nothing"; it does not validate "the feature is correct in all configurations." A dedicated test scene + RenderDoc capture remains the closure target — the toggle is the deferred-quality bridge, not the substitute.

A toggle introduced for this purpose has two possible lifecycles, decided at the point of introduction and recorded in the constants header next to the toggle's definition:

- **Transient** — the framing above. Delete the toggle once correctness is validated and the dedicated test scene exists; the toggle is scaffolding for a deferred quality check, not part of the engine's surface.
- **Durable** — the toggle is retained as a permanent architectural seam. Use this when the axis has ongoing A/B value (denoiser variants, integrator strategies, visualization modes), when reference paths future work might re-enable would otherwise bit-rot, or when the swappable layer is a designed seam in the engine's structure rather than a one-off experiment.

Every toggle must have either a documented retire criterion (transient) or a documented permanence rationale (durable) in the constants header. No toggles by inertia. The retire criterion is a measurable condition (e.g. "remove when on-path beats off-path on `<metric>` by `<margin>` across `<scenes>`"); the permanence rationale names the recurring need the toggle serves.

Compile-time vs runtime is a separate axis from transient vs durable. `#define` + `if constexpr` gives DXIL stripping and bit-identical output when off — the right surface for build-time-stable, audit-anchored seams. `DevToggleRegistry` gives live runtime flipping — the right surface for debug HUD / dev-menu / profile-driven flags. Two surfaces, two needs; do not conflate them.

Precedent for the durable variant: `PT_HASH_GRID_CACHE_ENABLED` in `Source/ExampleProject/RenderingClient/HashGridCacheConstants.h` (commit `84d14b92`) — the first deliberate use of the pattern as a permanent feature layer in this engine, retained for denoiser-comparison and reference-path preservation.

#### 3c — PT-as-ground-truth rasterizer comparison (separate use case)

When path-tracer output is the reference for **rasterizer** validation (not denoiser-vs-its-own-converged-output), the comparison is only fair on dimensions both pipelines compute. Before drawing conclusions from per-pixel deltas, enumerate what the rasterizer does NOT compute that PT does — unshadowed light types (e.g. point/sphere lights without shadow maps leak through walls in rast, get correctly occluded in PT), missing transmission/refraction, multi-bounce, missing post-effects, missing AA modes — and either disable those PT features for the comparison, or annotate the comparison artifact with the omitted-feature caveats.

Without this, a single rast-vs-PT delta conflates multiple bidirectional biases.

#### 3d — MAE-vs-CPU-PT is not the reference for PT denoising work

The `TestGIScene.ps1` MAE-vs-CPU-PT check exists as a regression guard against gross breakage of the rasterized GI path; it is **not** a quality-validation mechanism for path-tracer denoising, cache filtering, or post-PT composition. Reasons:

- CPU PT is pre-historical on the current pipeline and not trusted as ground truth on Sponza-scale scenes.
- MAE is a per-pixel scalar averaged spatially — it cannot see cell-pattern artifacts, just like temporal stddev cannot.
- The rasterizer-vs-PT framing is the wrong axis for "did denoising preserve quality"; the right axis is candidate-vs-its-own-converged-output (layer 3a).

Treat the MAE check as a sanity floor (catches "you broke the pipeline outright"), not as a quality bar.

### Layer 4 — User-eye fallback (the ratchet, not the default)

The user is the final visual arbiter. Their sign-off enters the closure record. Layer 4 fires when **any** of the following holds:

- The agent's layer-1 Verdict is `uncertain` (including the auto-`uncertain` from a missing candidate `Read`).
- The CL touches **composition**, **denoise**, **cache**, **tone-map**, **post-process**, or any **first-time-scene × first-time-feature** combination.
- The agent's layer-1 Verdict **disagrees with numeric metrics** — e.g. an AC numerically met but the agent reports a regression, or the agent reports an improvement that the numbers don't support.

On layer-4 trigger: pause closure, surface the layer-1 block + the relevant frames + the disagreement (if any) to the user, and wait for sign-off before claiming the AC. The sign-off — and any user-described differences — enters the closure record alongside the layer-1 block.

Layer 4 is the ratchet that catches the failure mode this discipline exists to prevent: numeric green shipping over visible regressions. It is not the default validation step — the goal is for layer 1 to catch the issue first. Layer 4 fires only when the agent itself flags doubt, the change category is high-risk, or the agent and the metrics disagree.

## Cross-references

- `peer-review-required.md` — the UNVERIFIED review tier exists for visual ACs that pass numerically but cannot actually be exercised by the test infra (auto-capture culling, headless harness limits). Reviewer must promote test-infra-blocked visual ACs to UNVERIFIED, not ADVISORY. Layer-1 absence on a rendering-output CL is itself a BLOCKED-tier finding. The reviewer-visual-inspection mandate (independent `Read` + structured block + `Reviewed-Visually:` commit footer) lives there; the `visual-review` commit-gate enforces the footer when the body references `Build/captures/`.
- `perf-measurement-frame-budget.md` — *visual-correctness verification* is explicitly out of scope for the perf-measurement N-picking rule; this discipline owns frame counts for visual claims.
- `regression-fix-flow.md` — when layer 1 or layer 4 catches a regression, the fix-up CL follows the regression-fix-flow.
- `dispatcher/dispatch-briefs.md` — when carry-forward advisories accumulate under the same backlog cross-reference (N > 1), the layered validation foundation has degraded; the dispatcher pauses and hands back rather than rolling into the next CL.
