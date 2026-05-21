// shadertype=hlsl
//
// TASK-138 — hardware-RT shadow rays for the sun. One thread per screen pixel;
// reads the GBuffer (world position + normal) at this pixel, traces a single
// cone-jittered shadow ray toward the sun, and writes a per-pixel float
// visibility (0 = shadowed, 1 = lit) consumed by LightPass::EvaluateSunLighting.
//
// Pattern reuse:
// - Ray flags + miss-shader-index 1 + ShadowPayload sentinel pattern from
//   PTRayGen.hlsl:260-266 (cite-prior-art). The same triplet
//   `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
//   RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` produces the cheapest opaque-only
//   visibility test the API supports — any opaque hit terminates the ray
//   with the closest-hit shader skipped (payload stays at its `true` init);
//   a miss runs ShadowMissShader (index 1) and flips the payload to false.
// - Cone-jitter via SampleSunDirection from common/sunSampling.hlsl. PT and
//   this pass produce identical sun-disc geometry — penumbra softness should
//   match modulo TAA convergence.
//
// MaxTraceRecursionDepth (DX12RenderPassResourceService.cpp:498) is 2; this
// pass dispatches depth-1 from the raygen — no further bump needed.

#include "common/common.hlsl"
#include "common/pathTracerPayload.hlsli"
#include "common/sunSampling.hlsl"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0)
{
	PerFrame_CB g_Frame;
}

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

// GBuffer position (RGB world-space, A=1 if surface present, 0 if sky).
[[vk::binding(1, 1)]]
Texture2D in_opaquePassRT0 : register(t1);

// GBuffer normal (RGB world-space).
[[vk::binding(2, 1)]]
Texture2D in_opaquePassRT1 : register(t2);

[[vk::binding(0, 2)]]
RWTexture2D<float> out_SunVisibility : register(u0);

// PCG-style hash for per-pixel jitter. Mixes pixel coords with frameIndex so
// TAA accumulation across frames produces soft-shadow penumbras over time
// without needing multiple samples per frame. Same hash family as
// SSRCClosestHit.hlsl::HitHash2D.
float2 PixelJitter2D(uint2 pixel, uint frameIndex)
{
	uint3 q = uint3(pixel.x, pixel.y, frameIndex) ^ uint3(0x68E31DA4u, 0xB5297A4Du, 0x1B56C4E9u);
	q = q * 1664525u + 1013904223u;
	q.x ^= q.y * q.z;
	q.y ^= q.z * q.x;
	q.z ^= q.x * q.y;
	q ^= q >> 16u;
	return float2((q.x & 0x00FFFFFFu) / float(0x01000000),
	              (q.y & 0x00FFFFFFu) / float(0x01000000));
}

[shader("raygeneration")]
void RayGenShader()
{
	uint2 pixel = DispatchRaysIndex().xy;

	float4 rt0 = in_opaquePassRT0.Load(int3(pixel, 0));
	if (rt0.w == 0.0)
	{
		// Sky pixel — no surface to shadow. Write 1 so downstream consumers
		// treat the texel as "fully lit" rather than read uninitialised
		// memory. LightPass::EvaluateSunLighting already early-outs on the
		// same sky test inside DecodeGBuffer, so this value is unconsumed
		// in practice — the explicit write is paranoia / GBV cleanliness.
		out_SunVisibility[pixel] = 1.0;
		return;
	}

	float3 positionWS = rt0.xyz;
	// RT1 stores the *shading* normal (normal-mapped, see
	// opaqueGeometryProcessPass.frag:127). With strong normal-map
	// perturbation the shading normal can deviate from the source
	// triangle's plane normal by tens of degrees, so a tight origin
	// offset along the shading normal can fail to escape the source
	// triangle (or land inside a neighbour triangle) — that reads as
	// Peter-Panning-style surface acne even though no shadow-map bias
	// is involved. The offset constant below is sized to absorb that
	// divergence.
	float3 normalWS   = normalize(in_opaquePassRT1.Load(int3(pixel, 0)).xyz);

	float3 sunDir = g_Frame.sun_direction.xyz;
	// Loud on data violations — feedback_no_data_integrity_assumptions.md.
	// A NaN or zero sun direction would produce undefined ray geometry; a
	// silent "fully lit" return would mask the bug. Fully-shadowed (0) is
	// the loud-failure visual: black image where there should be sun light,
	// distinct from lit-but-black-due-to-occlusion.
	float sunDirLenSq = dot(sunDir, sunDir);
	if (sunDirLenSq < 1e-8 || isnan(sunDirLenSq))
	{
		out_SunVisibility[pixel] = 0.0;
		return;
	}
	sunDir = sunDir * rsqrt(sunDirLenSq);

	// Per-pixel cone-jittered sun direction. One sample per pixel; TAA
	// accumulates across frames into a soft penumbra. Kept minimal here
	// (1 ray/pixel) so the cost-budget question — "RT shadow vs CSM+PCSS"
	// — is answered with the cheapest possible RT path. If TAA-off
	// becomes a supported config, bump samples here or add a small spatial
	// blur post-trace; deferred to follow-up per design alignment §"What
	// was NOT verified".
	float2 xi = PixelJitter2D(pixel, g_Frame.frameIndex);
	float3 lightDir = SampleSunDirection(sunDir, xi);

	ShadowPayload shadow;
	shadow.isShadowed = true;

	RayDesc shadowRay;
	// Larger normal offset than RAY_EPSILON (5 mm at the engine's
	// meter-scale scenes) absorbs the divergence between the GBuffer
	// shading normal (normal-mapped) and the actual triangle's geometric
	// normal — RAY_EPSILON = 0.001 m was tuned for PT, where the normal
	// passed in is the vertex-interpolated geometric normal and is
	// guaranteed to escape the source triangle.  Offset always +N so
	// that back-facing pixels (N·L < 0, shadowed by their own surface
	// in EvaluateSunLighting's BSDF clamp) self-shadow consistently in
	// the visibility texture too — flipping the offset sign there would
	// risk reading visibility=1 from open space behind the surface.
	// TMin remains RAY_EPSILON — once the origin is reliably above the
	// surface, an additional 1 mm along ray direction cheaply guards
	// against precision dust from the BVH near-hit logic.
	static const float SHADOW_RAY_NORMAL_OFFSET = 0.005f;
	shadowRay.Origin    = positionWS + normalWS * SHADOW_RAY_NORMAL_OFFSET;
	shadowRay.Direction = lightDir;
	shadowRay.TMin      = RAY_EPSILON;
	shadowRay.TMax      = RAY_MAX_DISTANCE;

	// MissShaderIndex = 1 → ShadowMissShader (registered second; primary
	// miss at index 0 is unused because RAY_FLAG_FORCE_OPAQUE +
	// RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH guarantee any hit terminates).
	TraceRay(SceneAS,
	         RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
	         0xFF, 0, 0, 1, shadowRay, shadow);

	out_SunVisibility[pixel] = shadow.isShadowed ? 0.0 : 1.0;
}
