// shadertype=hlsl
//
// TASK-138 — required-but-unused closest-hit shader for the sun-shadow RT
// pipeline. The raygen uses RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, so this body
// never executes; the symbol is required because the DXR PSO subobject set
// includes a closest-hit slot regardless. Same shape as PT's AnyHit — empty.

#include "common/pathTracerPayload.hlsli"

[shader("closesthit")]
void ClosestHitShader(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
}
