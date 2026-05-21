// shadertype=hlsl
//
// TASK-138 — shadow-miss shader (miss-shader index 1). Fires when the
// shadow ray reaches RAY_MAX_DISTANCE without hitting opaque geometry, i.e.
// the surface CAN see the sun. Flips the payload to "not shadowed".
// Identical shape to PTShadowMiss.hlsl.

#include "common/pathTracerPayload.hlsli"

[shader("miss")]
void ShadowMissShader(inout ShadowPayload payload)
{
	payload.isShadowed = false;
}
