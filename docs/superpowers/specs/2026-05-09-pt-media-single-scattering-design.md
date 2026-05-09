# PT-Side Participating Media (Single-Scattering) Design

## Goal

Add bounded homogeneous participating media to the GPU path tracer with single-scattering integration, delivering sun god-rays through dust as the primary user-visible target. PT-side replacement for the closed TASK-99 (raster-side volumetric, obsolete under PT-primary direction).

**Reference:** PBRT-v4 §11.4 *Equi-Angular Sampling for Single Scattering*; §11.2 *Henyey-Greenstein phase function*.

---

## Architecture

### New files

| File | Purpose |
|---|---|
| `Source/Engine/Component/MediumComponent.h` | ECS component declaration. Fields: `float sigma_t; float3 albedo; float g`. Bound = parent entity AABB. |
| `Source/Engine/Services/MediumDataService.h` | Service interface. Mirrors `LightDataService` shape. |
| `Source/Engine/Services/MediumDataService.cpp` | Per-frame collection of `MediumComponent` instances; pack into structured buffer; expose to PT pass. |
| `Source/ExampleProject/RenderingClient/PTMediaConstants.h` | `constexpr Inno::PT::Media::ENABLED`. No PT-medium code outside this `if constexpr` guard. |
| `Source/Shaders/HLSL/common/PTMediaIntegrator.hlsli` | Slab test, equiangular sample, Beer-Lambert, HG phase. Included from `PTRaygenIntegrator.hlsl`. |
| `Source/Shaders/HLSL/common/PTMediaBindings.hlsl` | StructuredBuffer slot for `MediumGPUData[]` + count. Mirror of `PTRaygenBindings.hlsl`. |

### Modified files

| File | Change |
|---|---|
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_BindingLayout.cpp` | Add slot for `t_MediumDataBuffer`. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Initialize.cpp` | Wire `MediumDataService` GPU upload to the binding slot. |
| `Source/ExampleProject/RenderingClient/GPUPathTracerPass_Update.cpp` | Per-frame `MediumDataService::PerFrameUpdate` dispatch hook. |
| `Source/Shaders/HLSL/common/PTRaygenIntegrator.hlsl` | Per-segment slab test + equiangular in-scatter call into `PTMediaIntegrator.hlsli`. Output to emissive radiance slot (see *Output channel routing*). |
| `Source/Engine/Common/DevToggleRegistry.h` | Add `g_DevToggle_PTMedia` (default ON). |

---

## Data shapes

### CPU component

Engine convention (verified against `LightComponent.h`): components are POD structs with static `GetTypeID()` and `GetTypeName()`, no inheritance. Member fields use `m_` prefix. `Vec4` used in preference to `Vec3` for 16-byte alignment.

```cpp
namespace Inno
{
    struct MediumComponent
    {
        static uint32_t   GetTypeID()   { return /* next free ID */; }
        static const char* GetTypeName() { return "MediumComponent"; }

        // Extinction coefficient sigma_t = sigma_a + sigma_s, per unit world distance.
        float m_SigmaT = 0.05f;

        // Single-scattering albedo = sigma_s / sigma_t. Warm-white default for dust.
        // .a unused; reserved for future per-channel sigma split.
        Vec4 m_Albedo = Vec4(1.0f, 0.95f, 0.85f, 0.0f);

        // Henyey-Greenstein anisotropy in [-1, 1]. 0 = isotropic; >0 = forward scatter.
        float m_HGAnisotropy = 0.5f;
    };
}
```

CL-1 first task includes: pick the next free `GetTypeID()` value and register `MediumComponent` in whatever component-registration site existing components use (read `LightComponent` registration as the reference).

Bounds: parent entity's transformed AABB via existing `TransformComponent` + the parent's mesh AABB (or, when no mesh, the entity's `WorldTransformComponent` extents). No new bound data on the component itself.

### GPU upload

```hlsl
struct MediumGPUData {
    float3 aabbMin;  float sigma_t;
    float3 aabbMax;  float g;
    float3 albedo;   float _pad;
};   // 48 bytes, naturally aligned
```

`StructuredBuffer<MediumGPUData> t_MediumDataBuffer` + `uint g_MediumCount` constant.

**Sigma model (unambiguous):** scalar `sigma_t` is the density (extinction per unit world distance, white). `albedo` is the per-channel fraction of scattered light surviving each scatter event. Derived quantities: `sigma_a = sigma_t · (1 - albedo)` (per-channel, white where albedo == 1), `sigma_s = sigma_t · albedo` (per-channel). Beer-Lambert transmittance is scalar (`T = exp(-sigma_t · d)`); in-scatter is colored (`sigma_s · L_in`). Adequate for warm-dust Sponza target; per-channel `sigma_t` is the chromatic-extinction follow-up axis (TASK-99.X if motivated).

**Maximum N volumes per frame:** 64 cap. CL-1 verifies whether the existing per-frame light list cap is similarly sized; if not, match it. Cap is checked in `MediumDataService` per-frame upload; overflow → Warning + truncate (deterministic by component-iteration order).

---

## Algorithm

### Per camera-ray segment (primary or bounce)

1. **Slab test** all `t_MediumDataBuffer[i]` AABBs against the segment `[origin, origin + t_max·dir]`. Collect intersection intervals `[t_enter_i, t_exit_i]`.
2. **Composite extinction** within each interval (overlapping volumes sum additively):
   - `composite_sigma_t   = Σ sigma_t_i`
   - `composite_sigma_s   = Σ (sigma_t_i · albedo_i)` (per-channel)
   - `composite_g         = (Σ sigma_s_i · g_i) / max(Σ sigma_s_i, eps)` (sigma-s-weighted mean)

   Implementation simplification: for CL-2 first pass, treat the whole segment as a single composite interval covering the *union* of all AABBs the segment passes through. Per-interval refinement is a CL-2.X follow-up if visual artifacts surface at volume boundaries.

3. **Beer-Lambert transmittance** along the segment:
   - `T_segment = exp(-composite_sigma_t · in_volume_distance)`
   - Apply to surface-hit radiance: `L_surface *= T_segment`.

### Equiangular in-scatter sample (sun NEE only)

At each segment that intersects ≥1 volume:

1. Pick the in-volume span `[t_a, t_b]` (union of intersection intervals on the segment).
2. **Equiangular sample** a distance `t_s ∈ [t_a, t_b]` per PBRT-v4 §11.4 (concentrating samples near the sun direction): given camera ray origin `O`, direction `D`, sun direction `L_sun`, point `O + t·D` whose closest distance to the light direction is minimized. Sample is canonical-uniform-warped to that minimum.
3. At in-scatter point `P = O + t_s·D`:
   - **Sun visibility** — trace shadow ray `P → P + L_sun · t_max`. If unoccluded:
     - **Transmittance through media along shadow ray** — slab test all volumes against shadow ray, accumulate `T_shadow = exp(-Σ sigma_t · in_volume_shadow_distance)`.
     - **Phase weight** — `phase = HenyeyGreenstein(dot(-D, L_sun), composite_g)` per PBRT-v4 §11.2.
     - **In-scatter contribution** — `L_inscatter = composite_sigma_s · sun_radiance · T_camera_to_P · T_shadow · phase / pdf_equiangular`.
4. Accumulate `L_inscatter` into the integrator's emissive radiance slot.

### Henyey-Greenstein

```hlsl
float HenyeyGreenstein(float cos_theta, float g) {
    float denom = 1 + g*g - 2*g*cos_theta;
    return (1 - g*g) / (4 * PI * denom * sqrt(denom));
}
```

### Equiangular sampling reference

PBRT-v4 §11.4. Canonical implementation:

```hlsl
// origin O, direction D, light point L
// segment [t_a, t_b] along the ray
float delta = dot(L - O, D);
float D_perp = length((L - O) - delta * D);
float theta_a = atan2(t_a - delta, D_perp);
float theta_b = atan2(t_b - delta, D_perp);
float t = delta + D_perp * tan(lerp(theta_a, theta_b, xi));
float pdf = D_perp / ((theta_b - theta_a) * (D_perp*D_perp + (t - delta)*(t - delta)));
```

For a directional sun light, `L` is a far point: `L = P_segment_midpoint + L_sun · 1e6f`. The directional limit reduces equiangular to standard distance sampling when `D_perp → ∞`; falls back gracefully without a special case.

---

## Output channel routing

Media in-scatter accumulates into a new **emissive** radiance slot — the "non-demodulated direct radiance" channel.

Rationale: NRD ReBLUR (incoming TASK-77.4 CL-2) demodulates diffuse and specular radiance by surface albedo at the integrator write site. Media in-scatter has no surface albedo to divide by; routing to diffuse/specular would corrupt the demod math (a `1 / max(albedo, eps)` divide on a value that has no associated albedo). Emissive bypasses NRD denoising entirely and composes into the final image post-tonemap.

**Verified state of the integrator (2026-05-09):** `Grep` for `emissive` across `Source/Shaders/HLSL/common/` returned zero matches in any PT shader. The integrator does not currently have a separate emissive radiance slot; surface self-emission (if any) is fused into one of the existing `radianceDiffuse` / `radianceSpecular` outputs.

**CL-1 scope (load-bearing):** introduce `radianceEmissive` as a third UAV write paralleling existing diffuse/specular outputs. Route any pre-existing direct-emission term there. Wire a new pass-binding slot in `GPUPathTracerPass` for the new RT. Composition (CL-3 of TASK-77.4 NRD work, when it lands) will read this slot post-NRD and add it to the final image. Until that lands, the CL-1 stub can route the new slot directly to the existing AccumBuffer alongside the diffuse/specular sum (single-buffered) — composition rewires when NRD's CL-3 absorbs the path.

---

## Toggles

### Compile-time

```cpp
// Source/ExampleProject/RenderingClient/PTMediaConstants.h
namespace Inno::PT::Media {
    constexpr bool ENABLED = true;
}
```

All medium code (service registration, GPU upload, integrator path) gated behind `if constexpr (Inno::PT::Media::ENABLED)`. Mirrors `Inno::PT::HashGridCache::ENABLED` and incoming `Inno::NRD::ENABLED`.

### Runtime

`g_DevToggle_PTMedia` in `DevToggleRegistry`. Default ON. When OFF, integrator skips slab tests + equiangular sampling; emissive media slot contributes zero. Allows in-engine A/B comparison without recompile.

---

## Test fixtures (CL-3)

≥1 `MediumComponent` in each of:

| Scene | Volume shape | Suggested defaults |
|---|---|---|
| `UnitTest.InnoScene` | Single AABB centered near camera, ~5×5×5 units | `sigma_t=0.1, albedo=(1,1,1), g=0` (isotropic, dense, easy sanity-check) |
| `GITestBox.InnoScene` | Thin slab inside the box | `sigma_t=0.05, albedo=(1,0.95,0.85), g=0.5` (showcase HG forward scatter) |
| `GISponza.InnoScene` | Large AABB encompassing the central nave interior | `sigma_t=0.02, albedo=(1,0.92,0.78), g=0.7` (warm dust, strong forward scatter for grazing-sun god-rays) |

Defaults are starting points. Tune in-engine via component inspector + DevToggle global density scale (CL-3 follow-up if needed).

---

## CL split

| CL | Scope | LoC | Visible-progress |
|---|---|---|---|
| **CL-1** | `MediumComponent` + `MediumDataService` + GPU binding + integrator stub returning zero in-scatter. Confirm or split the emissive radiance channel. Build green. | ~250 | None (no fixtures yet, integrator returns zero) |
| **CL-2** | `PTMediaIntegrator.hlsli`: slab test, equiangular sampling, Beer-Lambert, HG phase, sun NEE attenuation. Wire to integrator. | ~200 | First visible god-rays once any scene has a `MediumComponent` |
| **CL-3** | Test fixtures: 3 scenes get ≥1 `MediumComponent`. RenderDoc captures, visual sign-off, density tuning. | ~50 + scene authoring | User-visible showcase; closes TASK-99.1 |

Total: ~500 LoC across 3 commits + 3 scene-data edits.

---

## Out of scope (explicit)

- **Heterogeneous media** (NanoVDB, 3D-texture density grids) — different shape (sampled densities, hierarchical DDA traversal). File as TASK-99.2 if motivated.
- **Distance-sampled MC for non-sun in-scatter** (sky / radiance-cache contribution as in-scatter sources) — extends to general light sampling at the in-scatter point. File as TASK-99.3 if Sponza-style indoor showcase grows beyond sun-driven.
- **Custom editor inspector** for `MediumComponent` — stock component-add UX only; component appears in the engine's existing component-list once registered.
- **Aerial perspective / atmospheric scattering** — different shape (screen-space + sky-physics, height-fall-off density). TASK-104 / TASK-105 territory.
- **Multiple-scattering MC** (random-walk through media) — overkill for Sponza-target. Subsurface / cloud-grade work; revisit only if heterogeneous media (TASK-99.2) lands first.
- **Mesh-bound volumes** (ray-vs-arbitrary-mesh as the medium boundary) — AABB-bound only. Arbitrary mesh boundaries require BVH-style traversal in the integrator hot path; not justified for the showcase.

---

## Risks

| Risk | Mitigation |
|---|---|
| Emissive channel does not exist (verified 2026-05-09 — `Grep` for `emissive` in PT shaders returned zero) | CL-1 introduces it as a new UAV write paralleling diffuse/specular. Until NRD CL-3 composition lands, CL-1 stub routes it directly into the AccumBuffer sum. |
| Per-segment composite sigma loses precision at volume boundaries (one volume's edge inside another) | CL-2 first pass uses union-interval simplification; per-interval refinement (per-volume contribution stratified along the segment) deferred to CL-2.X if visual artifacts surface. |
| Equiangular variance at near-grazing or near-aligned sun angles | PBRT-v4's recipe is variance-stable in practice; if not, MIS combine with uniform distance sampling (CL-2 follow-up). |
| Multiple overlapping volumes producing over-dense interiors | Per-volume `sigma_t` is tunable; CL-3 includes a runtime DevToggle for global density scalar if the per-scene authoring becomes annoying. |
| Interaction with NRD ReBLUR when TASK-77.4 CL-2 lands | None by construction. Emissive channel routes around NRD; the demod math operates only on diffuse/specular slots. |
| 64-volume cap overflowed by some future scene | Cap checked at upload (see *Data shapes / GPU upload*); overflow → Warning + truncate, deterministic by iteration order. |
| `MediumComponent`'s parent transform changes per-frame (animated entity) | Bounds re-uploaded each frame from current world-transformed AABB; no caching. Same as existing renderable component lifecycle. |

---

## Cross-references

- **TASK-99** — predecessor, closed obsolete-under-PT-primary 2026-05-09. This spec is the PT-side replacement.
- **TASK-77** — PT-primary direction parent.
- **TASK-77.4** — concurrent NRD ReBLUR integration. Emissive channel routing is designed to coexist (no NRD interaction).
- **TASK-104** / **TASK-105** — sky / cloud / sun R&D; out of scope here, different shape.
- **PBRT-v4 §11.2 / §11.4** — phase function + equiangular sampling references.
