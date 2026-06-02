# Project direction

## Rendering (2026-05-22)

PT + NRD ReBLUR is the keeper denoise path. GPU path tracer + NRD produces acceptable output with motion fireflies (user direction layer-4).

SSRC GI 1.0 port (TASK-226) remains in tree at paper-divergent state — Capsaicin SampleScreenProbes (`gi1.comp:481-553` 64-rays + workgroup-parallel CDF) not landed; current 16-rays + per-thread CDF. Quality: below Capsaicin reference; TASK-226 closed with honest divergence record.

PT hash-grid cache + SSRC + post-SSRC denoise (SSRCTemporal / SSRCSpatialH/V) all run concurrently in default pipeline. No formal rasterizer-demotion-as-default landed; multi-stack coexistence is the de-facto state.

## Naming convention

- PT prefix → GPU path tracer code/shaders
- SSRC prefix → screen-space radiance cache (GI 1.0 port)
- SSRCTemporal / SSRCSpatial → post-SSRC denoise trio (was GIDenoise / GIFilter)
