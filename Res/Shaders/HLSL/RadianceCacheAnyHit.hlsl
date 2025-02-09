// shadertype=hlsl
#include "RayTracingCommon.hlsl"

[shader("anyhit")]
void AnyHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // For now, simply accept the hit.
    // Later, you might call IgnoreHit() if you wish to discard certain intersections.
}
