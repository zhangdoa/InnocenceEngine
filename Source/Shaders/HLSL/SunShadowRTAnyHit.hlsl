// shadertype=hlsl
//
// TASK-138 — required-but-unused any-hit shader for the sun-shadow RT
// pipeline. RAY_FLAG_FORCE_OPAQUE skips it; the symbol is mandated by the
// PSO subobject set. Same shape as PTAnyHit.hlsl.

#include "common/pathTracerPayload.hlsli"

[shader("anyhit")]
void AnyHitShader(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
}
