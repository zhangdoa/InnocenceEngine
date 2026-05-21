// shadertype=hlsl
//
// TASK-138 — primary-miss stub for the sun-shadow RT pipeline. The
// shadow-ray dispatch in SunShadowRTRayGen.hlsl uses MissShaderIndex=1
// (ShadowMissShader), so this miss never fires; it exists only because
// the PSO requires a miss-shader at index 0. Empty body matches the
// "unused but required" precedent of PTMiss.hlsl behaviour for
// shadow-only pipelines.

#include "common/pathTracerPayload.hlsli"

[shader("miss")]
void MissShader(inout ShadowPayload payload)
{
}
