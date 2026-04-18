// shadertype=hlsl
#ifndef PATH_TRACER_PAYLOAD_HLSLI
#define PATH_TRACER_PAYLOAD_HLSLI

// DXR gives raygen, miss, and closest-hit one flat payload blob — any field
// here must be visible to all three stages, otherwise a write in one stage
// silently clobbers a different field in another stage's view. Put the struct
// here, #include it everywhere; don't copy-paste.
//
// Size is mirrored by MaxPayloadSizeInBytes in DX12RenderPassResourceService.cpp
// (currently 64). If you add a field, check both.
struct PathTracerPayload
{
    float3 hitPos;     // 12B
    float3 normal;     // 12B
    float2 texCoord;   //  8B
    float3 albedo;     // 12B
    float  metalness;  //  4B
    float  roughness;  //  4B
    bool   missed;     //  4B, HLSL pads to 4
};                     // 56B total; MaxPayloadSizeInBytes = 64 for alignment.

struct ShadowPayload { bool isShadowed; };

#endif // PATH_TRACER_PAYLOAD_HLSLI
