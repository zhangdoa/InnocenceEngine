# Shaders

- `HLSL/` — engine HLSL. Compiled to DXIL by `Scripts/HLSL2DXIL.ps1`.
- `HLSL/common/` — shared headers / generic helpers.
- `MSL/` — legacy Metal sources (build-disabled).

Naming: PT prefix → GPU path tracer. SSRC prefix → screen-space radiance cache (GI 1.0 port).
