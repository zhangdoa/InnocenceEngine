---
id: TASK-76
title: >-
  Path tracer curtain colors come out inverted (red→cyan, green→gold) — likely
  sRGB or channel-order issue
status: To Do
assignee: []
created_date: '2026-04-18 19:24'
labels:
  - bug
  - path-tracer
  - colorspace
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
After TASK-74 fix (texture sampling actually reaches the path tracer shader), Sponza curtains render with visible texture patterns but the hues are wrong: curtain_fabric_red_BaseColor shows as cyan/turquoise; curtain_fabric_green_BaseColor shows as gold. Each channel appears complemented — classic color-negation pattern, not a swap.

The source PNG (`OriginalAssets/Models/Sponza_Curtains/pkg_a_curtains/textures/curtain_fabric_red_BaseColor.png`) is clearly bright red, so the on-disk asset is correct. Something in the pipeline between "BC1-compressed red texture on disk" and "albedo fed to BRDF" inverts the hue.

Hypothesis candidates:
- **Specular-on-metal-at-grazing**: with the compounded scalar defaults (`metalness=1 roughness=1` for every glTF surface because those are the spec defaults when no factor is given), Sponza curtains render as chrome. Chrome reflects ENVIRONMENT colors, not its own albedo. At grazing angles Fresnel→1 (white) regardless of F0. The "cyan/gold" could be the skylight reflection painted onto chrome curtains — not actually the curtain color at all. Check: override metalness=0 in the shader and see if curtain hue changes to match the source PNG.
- **sRGB view-format mismatch**: if the SRV is created as `BC1_UNORM` instead of `BC1_UNORM_SRGB`, the shader reads gamma-encoded values as linear — washes darks but doesn't invert hues. Unlikely to be the full story here, but worth verifying.
- **kD = (1-F)*(1-metalness) with F tinted by F0=albedo**: for dielectric paths (if any get taken), `kD` effectively becomes `(1 - albedo) * 1 = complement(albedo)`, which IS exactly the observed color inversion. Could be happening if metalness resolves to 0 post-fix for some pixels.

Verification plan:
1. Temp shader: force `metalness = 0.0` at primary hit, render Sponza. If curtains become red/green/blue, the chrome hypothesis wins — fix the asset-import metalness scalar default or ship a better glTF path in `AssimpMaterialProcessor::ProcessMaterialProperties`.
2. Temp shader: output `payload.albedo` directly (raw texture sample). If that's red/green/blue, the shader's BRDF math is introducing the inversion, not the texture.

Likely resolution: glTF defaults to metalness=1 / roughness=1 when factors aren't specified, but practically almost no mesh that needs PBR metallic-roughness leaves both unset — the spec default was chosen wrong for real-world use. Sensible override: when neither `AI_MATKEY_METALLIC_FACTOR` nor an MR texture is present, default to dielectric (0). This tweak in `AssimpMaterialProcessor` would fix the entire Sponza scene's material interpretation and likely resolve the apparent "color inversion" as a side effect.
<!-- SECTION:DESCRIPTION:END -->
