// shadertype=hlsl
//
// LightPass direct-lighting evaluators: sun + tiled point lights.
// Each evaluator writes into the (direct, indirect-seed) accumulator
// pair via AccumulateLightContribution from lightPassCommon.hlsl.

#ifndef LIGHTPASS_DIRECT_LIGHTING_HLSL
#define LIGHTPASS_DIRECT_LIGHTING_HLSL

// Sun light. Constructs the directional light vector with sun-disc
// clamping (when V points inside the sun disc, L collapses to V), runs
// the BSDF accumulator, then attenuates by sun shadow visibility from
// SunShadowRTPass (hardware-RT cone-jittered ray, R8 unorm; 0=shadowed,
// 1=lit). TAA accumulates the per-frame jittered samples into a soft
// penumbra.
//
// io_DirectLuminance and io_IndirectSeedLuminance are accumulators —
// the function adds the sun's contribution, it does not overwrite.
void EvaluateSunLighting(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in Texture2D<float> in_SunShadowRTVisibility,
	in MaterialAttributes in_Material,
	in float3 in_PositionWS,
	in float3 in_NormalWS,
	in float3 in_V,
	in uint2 in_ScreenCoord,
	inout float3 io_DirectLuminance,
	inout float3 io_IndirectSeedLuminance)
{
	// Sun-disc clamp: when the view ray lands inside the sun disc, treat
	// the light direction as V (the sun fills the BRDF lobe). Otherwise
	// take the sun direction tilted toward V by the disc radius.
	float3 D = normalize(g_Frame.sun_direction.xyz);
	float r = sin(SUN_ANGULAR_RADIUS);
	float d = cos(SUN_ANGULAR_RADIUS);
	float DdotV = dot(D, in_V);
	float3 S = in_V - DdotV * D;
	float3 L = DdotV < d ? normalize(d * D + normalize(S) * r) : in_V;

	float3 l_SunDirect = float3(0.0, 0.0, 0.0);
	float3 l_SunIndirectSeed = float3(0.0, 0.0, 0.0);
	AccumulateLightContribution(
		in_BRDFLUT, in_BRDFMSLUT, in_PointSampler,
		in_Material, in_V, in_NormalWS, L,
		g_Frame.sun_illuminance.xyz, 1.0,
		l_SunDirect, l_SunIndirectSeed);

	float l_Visibility = in_SunShadowRTVisibility.Load(int3(in_ScreenCoord, 0));
	io_DirectLuminance += l_SunDirect * l_Visibility;
	io_IndirectSeedLuminance += l_SunIndirectSeed * l_Visibility;
}

// Tiled point lights. Looks the visible-light list for this tile out of
// the LightCullingPass grid (LIGHT_CULLING_BLOCK_SIZE-pixel tiles), then
// runs the BSDF accumulator for each.
//
// TASK-176 — per-light visibility is an inline RayQuery<> shadow ray from
// the surface point toward the light position. SM 6.5 / DXR Tier 1.1; the
// shader profile is pinned to cs_6_5 in Scripts/Lib/Compile-HLSL.psm1.
// Pattern reused from SunShadowRTRayGen.hlsl (cite-prior-art): same
// `RAY_FLAG_FORCE_OPAQUE | ACCEPT_FIRST_HIT_AND_END_SEARCH |
// SKIP_CLOSEST_HIT_SHADER` triplet — the cheapest opaque-only visibility
// query the API supports. Inline RT replaces the TraceRay / payload dance
// with a stack-local RayQuery<> that we Proceed() once and read back.
//
// l_PointLight.shadow.x gates the trace — set by LightDataService from
// LightComponent::m_CastShadow (TASK-149). Off → skip the ray, visibility = 1.
//
// Self-shadow nudge: EPSILON * in_NormalWS is the project convention.
// SunShadowRT uses a larger 5 mm offset because that pass passes the
// *shading* normal (normal-mapped, can deviate ~tens of degrees from the
// triangle plane). LightPass already runs with the GBuffer shading normal
// here too — keep the same offset shape so acne behaviour is uniform with
// the sun path. RAY_EPSILON is then enforced as TMin.
void EvaluateTiledPointLighting(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in Texture2D<uint2> in_LightGrid,
	in StructuredBuffer<uint> in_LightIndexList,
	in RaytracingAccelerationStructure in_SceneAS,
	in MaterialAttributes in_Material,
	in float3 in_PositionWS,
	in float3 in_NormalWS,
	in float3 in_V,
	in uint2 in_ScreenCoord,
	inout float3 io_DirectLuminance,
	inout float3 io_IndirectSeedLuminance)
{
	uint2 l_TileIndex = uint2(floor((float2)in_ScreenCoord / LIGHT_CULLING_BLOCK_SIZE));
	uint l_StartOffset = in_LightGrid[l_TileIndex].x;
	uint l_LightCount = in_LightGrid[l_TileIndex].y;

	// Same 5 mm normal offset as SunShadowRTRayGen to absorb shading-vs-
	// geometric normal divergence; both consumers read the GBuffer shading
	// normal, so the offset must agree to avoid asymmetric self-shadow acne.
	const float SHADOW_RAY_NORMAL_OFFSET = 0.005f;
	float3 l_RayOrigin = in_PositionWS + in_NormalWS * SHADOW_RAY_NORMAL_OFFSET;

	[loop]
	for (uint i = 0; i < l_LightCount; ++i)
	{
		uint l_LightIndex = in_LightIndexList[l_StartOffset + i];
		PointLight_CB l_PointLight = g_PointLights[l_LightIndex];

		float3 L_unnormalized = l_PointLight.position.xyz - in_PositionWS;
		float l_Distance = length(L_unnormalized);
		float l_AttenuationRadius = l_PointLight.luminousFlux.w;
		float3 L = L_unnormalized * (1.0 / max(l_Distance, EPSILON));

		float l_InvSquareRadius = 1.0 / max(l_AttenuationRadius * l_AttenuationRadius, EPSILON);
		float l_AttenuationFactor = CalculateDistanceAttenuation(L_unnormalized, l_InvSquareRadius);

		float3 l_LightDirect = float3(0.0, 0.0, 0.0);
		float3 l_LightIndirectSeed = float3(0.0, 0.0, 0.0);
		AccumulateLightContribution(
			in_BRDFLUT, in_BRDFMSLUT, in_PointSampler,
			in_Material, in_V, in_NormalWS, L,
			l_PointLight.luminousFlux.xyz, l_AttenuationFactor,
			l_LightDirect, l_LightIndirectSeed);

		// TASK-176 inline RT shadow.
		// PerFrame_CB.pointShadowBypass forces visibility=1 for visual A/B —
		// flip the runtime DevToggle (PointShadowBypass) on the editor side
		// to confirm shadow contribution is the only rendered-output delta
		// vs an unshadowed point light. TASK-195 migrated this from the
		// former #define DEBUG_POINT_SHADOW_BYPASS; precedent on the cube
		// path (TASK-148, commit 71817f3a) per
		// .claude/disciplines/visual-validation.md A/B-toggle pattern.
		float l_Visibility = 1.0;
		if (g_Frame.pointShadowBypass == 0u && l_PointLight.shadow.x != 0u)
		{
			RayDesc l_ShadowRay;
			l_ShadowRay.Origin    = l_RayOrigin;
			l_ShadowRay.Direction = L;
			l_ShadowRay.TMin      = RAY_EPSILON;
			// TMax = surface→light distance, minus the normal offset so we
			// don't overshoot past the light. Light itself is not in the BVH.
			l_ShadowRay.TMax      = max(l_Distance - SHADOW_RAY_NORMAL_OFFSET, RAY_EPSILON);

			RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER> l_Query;
			l_Query.TraceRayInline(in_SceneAS, 0, 0xFF, l_ShadowRay);
			l_Query.Proceed();
			// COMMITTED_NOTHING (= miss with FORCE_OPAQUE+ACCEPT_FIRST_HIT)
			// means no occluder between origin and light → visible.
			l_Visibility = (l_Query.CommittedStatus() == COMMITTED_NOTHING) ? 1.0 : 0.0;
		}
		io_DirectLuminance += l_LightDirect * l_Visibility;
		io_IndirectSeedLuminance += l_LightIndirectSeed * l_Visibility;
	}
}

#endif // LIGHTPASS_DIRECT_LIGHTING_HLSL
