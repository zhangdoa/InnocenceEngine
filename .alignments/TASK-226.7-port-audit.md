# TASK-226.7 — InterpolateScreenProbes bent-cone SH eval: audit + deferred-port closure

Phase 1.7 of TASK-226 umbrella. Gap-matrix row #7. The bent-cone math is fully extracted and ready to port. The actual port is blocked on an upstream pipeline-ordering question (AO source not available at the eval site). Closing audit-only; reopen when AO source is plumbed.

## Capsaicin's bent-cone evaluation

`ScreenProbes_CalculateSHIrradiance_BentCone(normal, ao, probe)` in `src/core/src/render_techniques/gi1/screen_probes.hlsl`:

```hlsl
float clamped_cosine_sh[9];
SH_GetCoefficients_ClampedCosine_Cone(
    normal, acos(sqrt(saturate(1.0f - ao))), clamped_cosine_sh);

uint probe_index = probe.x + probe.y * probe_count;
float3 irradiance = float3(0.0f, 0.0f, 0.0f);

for (uint i = 0; i < 9; ++i)
{
    irradiance += clamped_cosine_sh[i]
                * ScreenProbes_UnpackSHColor(
                    g_ScreenProbes_ProbeSHBuffer[9 * probe_index + i]).xyz;
}

return max(irradiance, 0.0f);
```

`SH_GetCoefficients_ClampedCosine_Cone` (math/spherical_harmonics.hlsl), fully extracted:

```hlsl
void SH_GetCoefficients_ClampedCosine_Cone(in float3 cosine_lobe_dir,
                                            in float cone_theta_max,
                                            out float coefficients[9])
{
    float sin_theta_max, cos_theta_max;
    sincos(cone_theta_max, sin_theta_max, cos_theta_max);
    float sin_theta_max2 = sin_theta_max * sin_theta_max;
    float sin_theta_max3 = sin_theta_max2 * sin_theta_max;
    float cos_theta_max2 = cos_theta_max * cos_theta_max;
    float cos_theta_max3 = cos_theta_max2 * cos_theta_max;

    float band1_factor = 1.023326707946489f * (1.0f - cos_theta_max3);
    float band2_factor = (4.0f - 3.0f * sin_theta_max3) * sin_theta_max2;

    coefficients[0] = 0.886226925452758f * sin_theta_max2;
    coefficients[1] = -band1_factor * cosine_lobe_dir.y;
    coefficients[2] = +band1_factor * cosine_lobe_dir.z;
    coefficients[3] = -band1_factor * cosine_lobe_dir.x;
    coefficients[4] = +0.8580855308097834f * band2_factor * cosine_lobe_dir.x * cosine_lobe_dir.y;
    coefficients[5] = -0.8580855308097834f * band2_factor * cosine_lobe_dir.y * cosine_lobe_dir.z;
    coefficients[6] = +0.2477079561003757f * band2_factor * (3.0f * cosine_lobe_dir.z * cosine_lobe_dir.z - 1.0f);
    coefficients[7] = -0.8580855308097834f * band2_factor * cosine_lobe_dir.x * cosine_lobe_dir.z;
    coefficients[8] = +0.4290427654048917f * band2_factor * (cosine_lobe_dir.x * cosine_lobe_dir.x - cosine_lobe_dir.y * cosine_lobe_dir.y);
}
```

When `ao = 1.0` → `theta_max = π/2`: sin=1, cos=0, sin³=1, band1_factor=1.0233, band2_factor=1, coefficient[0]=0.8862. These degenerate to the standard Ramamoorthi-Hanrahan cosine-lobe zonal-harmonic coefficients = our current `LoadIrradiance` behavior.

## Our current eval

`common/RadianceCacheCommon.hlsl:183-205` (`LoadIrradiance`):

```hlsl
const float A0 = 1.0;            // unnormalised: 0.886/Y00 normalisation absorbed
const float A1 = 2.0 / 3.0;      // unnormalised: 1.023/Y1x normalisation absorbed
const float A2 = 1.0 / 4.0;      // unnormalised: 0.858/Y2x normalisation absorbed

float3 band0 = A0 * c00 * Y_00();
float3 band1 = A1 * (c11 * Y_11(n) + c1_1 * Y_1_1(n) + c10 * Y_10(n));
float3 band2 = A2 * (c2_2 * Y_2_2(n) + c2_1 * Y_2_1(n) + c20 * Y_20(n)
                   + c21 * Y_21(n) + c22 * Y_22(n));
return max(band0 + band1 + band2, 0.0);
```

Conventions differ — we absorb the SH basis-function normalisation into the stored coefficient + the A_n factor; Capsaicin keeps everything explicit. Equivalent at `theta_max = π/2` modulo this convention difference (which is consistent between our `RadianceCacheIntegration.comp` projection step and `LoadIrradiance` eval).

## Blocker — AO source pipeline ordering

Capsaicin's `ao` source is `g_OcclusionAndBentNormalBuffer.w` (gi1.comp:1622-1630). Bound at InterpolateScreenProbes, which runs AFTER their AO pass.

Our pipeline:
- `ExampleRenderingClient_ExecuteCommands_Rasterizer.cpp:71` calls `ExecuteGIPasses()` (= RadianceCache* + GIDenoise + GIFilter).
- `:73` then dispatches `SSAOPass`.
- `:124` then dispatches `LightPass` waiting on SSAO.

`GIDenoise.comp:170` calls `SampleRadianceCache` → `LoadIrradiance` per-pixel — BEFORE SSAOPass writes its output. Binding SSAO into GIDenoise creates a dependency cycle.

## Options to unblock

1. **Move `SSAOPass` to run before `ExecuteGIPasses`**. Re-check all downstream SSAO consumers (lightPass.comp:72 reads `in_SSAO`). Pure dispatch-order change; no algorithmic change. Architectural decision — should land as its own CL with peer review.
2. **Previous-frame SSAO via ping-pong**. 1-frame latency on AO; acceptable for low-frequency occlusion signal. Adds ping-pong scaffolding (cf. TASK-128, currently demoted).
3. **`ao = 1.0` placeholder**. Adds the AO parameter to `LoadIrradiance` + introduces the bent-cone math, but the formula is mathematically identical to current at `ao = 1.0`. Busywork until the AO source plumbs in — per user goal "no busywork or overscope."

Option (1) is cleanest. Either it folds into the TASK-227 declarative render-graph effort (graph compiler would emit the dispatch order from data, so reordering is a one-line config change) or it's its own task.

## What was NOT verified

- The convention difference between Capsaicin's explicit-band-factors and our absorbed-normalisation form was not numerically verified for `theta_max ≠ π/2`. Porting needs a unit test or careful inspection that the SH coefficient storage convention in `RadianceCacheIntegration.comp` matches what `SH_GetCoefficients_ClampedCosine_Cone` expects.
- The actual port quality benefit (small-scale occlusion sharpening — paper Figure 9 region) is contingent on the AO source plumbing landing; not measurable until then.

## Closure-Reason

Audit done; math extracted and ready. Port deferred pending pipeline-order resolution. Not "defer to a subtask" — concrete blocker surfaced; reopen this task when the AO source is available at GIDenoise time.
