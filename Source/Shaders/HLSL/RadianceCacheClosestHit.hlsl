// shadertype=hlsl
#include "RayTracingBindings.hlsl"
#include "common/BSDF.hlsl"

// TASK-6.10 sky NEE at the secondary vertex. The radiance-cache closest-hit
// pipeline does not bind material buffers (vertex/index/material heaps are
// opaque-pass / GPU-PT exclusive — see RadianceCacheRaytracingPass.cpp slot
// list), so the secondary surface's true albedo is not available without
// new bindings. Use a fixed mid-grey Lambertian approximation; routing
// per-hit material is a separate task that lifts this constant to the true
// per-instance albedo from in_MaterialBuffer[InstanceID()] — same pattern
// PT's GPUPathTracerClosestHit follows. The conservative 0.5 keeps the
// estimator from doubling once material routing lands and the world cache
// starts seeing real (often higher) reflectance.
static const float3 SECONDARY_VERTEX_ALBEDO_FALLBACK = float3(0.5, 0.5, 0.5);

float2 NDCToScreenSpace(float2 positionNDC)
{
    // Convert to screen space (0 to 1)
    float2 screenCoord = positionNDC * 0.5 + 0.5;

    // Flip Y for screen-space
    screenCoord.y = 1.0 - screenCoord.y;

    // Convert to pixel space (0 to viewportSize)
    screenCoord *= g_Frame.viewportSize.xy;

    return screenCoord;
}

float4 ClipToNDC(float4 positionCS)
{
    float w = max(abs(positionCS.w), EPSILON);
    return positionCS / w;
}

float2 ClipToScreenSpace(float4 positionCS)
{
    float4 positionNDC = ClipToNDC(positionCS);
    return NDCToScreenSpace(positionNDC.xy);
}

// PCG-style hash for per-hit RNG. Position quantised to ~mm so adjacent
// closest-hit invocations on the same surface share a seed (temporal
// reuse), but distinct hit points decorrelate. Mixed with frame index
// so consecutive frames pick different sky directions even when the
// probe ray family is stable.
float2 HitHash2D(float3 hitPositionWS, uint frameIndex)
{
    uint3 q = uint3(int3(hitPositionWS * 1024.0)) ^ uint3(0x68E31DA4u, 0xB5297A4Du, 0x1B56C4E9u);
    q.x ^= frameIndex * 0x9E3779B9u;
    q.y ^= frameIndex * 0x85EBCA77u;
    q.z ^= frameIndex * 0xC2B2AE3Du;
    uint3 v = _PCG3D(q);
    return float2((v.x & 0x00FFFFFFu) / float(0x01000000),
                  (v.y & 0x00FFFFFFu) / float(0x01000000));
}

float3 CosineSampleHemisphereTangent(float2 Xi, float3 N)
{
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt(1.0 - Xi.y);
    float sinTheta = sqrt(Xi.y);
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3x3 basis = CreateTangentSpace(N);
    return normalize(mul(H, basis));
}

// TASK-6.10 sky NEE at the secondary vertex. Estimates hemispheric sky
// irradiance reaching the hit surface via a one-sample Monte-Carlo
// integrator, with the radiance-cache's per-probe stratified samples and
// temporal blend providing the variance reduction.
//
// Convention vs GPU PT (GPUPathTracerRayGen.hlsl sky-NEE block):
//   - PT has the surface shading normal in payload.normal so it samples
//     the hemisphere over N (uniform), evaluates CookTorranceGGX(N,V,L)
//     which returns BRDF·cos, and multiplies by 1/pdf = 2π. Estimator:
//       radiance += throughput · CookTorrance · skyColor · 2π
//   - The radiance-cache RT pipeline does NOT bind vertex/index buffers
//     and the closest-hit barycentrics on their own cannot reconstruct
//     a world-space shading normal without ObjectToWorld vertex data.
//     Sampling a hemisphere over `-WorldRayDirection()` (the obvious
//     proxy) is wrong: probe rays hit ceilings/walls, so the incoming
//     ray reverse points DOWN and a hemisphere over it samples into the
//     floor — exactly the wrong half of space for sky NEE.
//   - Workaround: bias sampling around world-up (0,1,0) and let
//     visibility (the shadow-ray miss test) gate which directions
//     actually see sky. Surfaces facing the open atrium roof get full
//     sky energy, surfaces blocked by ceiling get zero. Acts as a
//     cosine-weighted estimator of `∫_upper-world-hemi skyColor · V dω`,
//     which equals the real Lambertian sky irradiance for surfaces with
//     N ≈ +Y and over-estimates moderately for horizontal walls — the
//     same conservative bias Capsaicin's GI-1.0 reference takes when
//     environment is sampled without a per-vertex BRDF (gi1.comp:1962).
//   - Cosine-weighted PDF over (0,1,0): pdf(L) = (Y·L)/π, L in upper-Y
//     hemisphere. Lambertian BRDF·cos / pdf collapses to `albedo` per
//     the standard cosine-importance cancellation. Mid-grey
//     SECONDARY_VERTEX_ALBEDO_FALLBACK stands in for the true albedo
//     until material binding plumbing lands (cross-ref TASK-6.8 #7/D4).
//   - Visibility: RAY_FLAG_SKIP_CLOSEST_HIT_SHADER plus a payload-sentinel
//     pattern (-1.0 = no miss-shader fired). The miss shader writes
//     payload.radiance = sky color and payload.distance = RayTCurrent(),
//     so a successful sky hit returns sky color directly with no second
//     miss-shader / shadow-payload subtype needed.
float3 SampleSkyNEE(float3 hitPositionWS, uint frameIndex)
{
    // Bias the hemisphere sampler around world-up. See function comment.
    const float3 worldUp = float3(0.0, 1.0, 0.0);

    float2 xi = HitHash2D(hitPositionWS, frameIndex);
    float3 skyDir = CosineSampleHemisphereTangent(xi, worldUp);
    if (skyDir.y <= 0.0)
        return float3(0, 0, 0);     // numerical edge — sample fell below the horizon, drop

    RayPayload skyPayload;
    skyPayload.radiance = float3(0, 0, 0);
    skyPayload.distance = -1.0;     // sentinel: miss shader overwrites with RayTCurrent() ≥ 0

    RayDesc skyRay;
    skyRay.Origin    = hitPositionWS + worldUp * RAY_EPSILON;
    skyRay.Direction = skyDir;
    skyRay.TMin      = RAY_EPSILON;
    skyRay.TMax      = RAY_MAX_DISTANCE;

    // RAY_FLAG_SKIP_CLOSEST_HIT_SHADER + RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH:
    // any opaque hit terminates the ray with the closest-hit shader skipped, so
    // payload stays at its sentinel. A miss runs the existing RadianceCacheMiss
    // and writes sky colour + RayTCurrent() into the payload. The post-trace
    // sentinel test (distance >= 0) distinguishes the two without a separate
    // shadow-payload struct or a second miss shader.
    TraceRay(SceneAS,
             RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF, 0, 0, 0, skyRay, skyPayload);

    if (skyPayload.distance < 0.0)
        return float3(0, 0, 0);     // occluded — secondary surface saw geometry, not sky

    float3 skyColor = skyPayload.radiance;
    if (any(isnan(skyColor)) || any(isinf(skyColor)))
        return float3(0, 0, 0);     // miss-shader produced NaN/Inf — drop, do not contaminate cache

    return skyColor * SECONDARY_VERTEX_ALBEDO_FALLBACK;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // Compute hit position in world space
    float3 hitPositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Compute screen space coordinate from hitPositionWS
    float4 positionVS = mul(float4(hitPositionWS, 1.0), g_Frame.v);
    float4 positionCS = mul(positionVS, g_Frame.p_original);
    float2 screenCoord = ClipToScreenSpace(positionCS);

    float2 motionVector = in_opaquePassRT3.Load(int3(screenCoord, 0)).xy;
    float2 prevScreenCoord = screenCoord + motionVector;

    // Fetch previous frame the light pass radiance
    float3 hitRadiance = float3(0, 0, 0);
    bool withinBounds = all(prevScreenCoord >= float2(0, 0)) && all(prevScreenCoord <= g_Frame.viewportSize.xy);
    if (withinBounds)
    {
        // Read the Lambertian diffuse outgoing radiance (albedo * E / PI) from the previous frame.
        // Lambertian radiance is view-independent, so no NdotV factor.
        // Radiance is constant along a ray, so no distance attenuation.
        hitRadiance = in_LightPassOutgoingLuminance.Load(int3(prevScreenCoord, 0)).rgb;

        if (any(isnan(hitRadiance)) || any(isinf(hitRadiance)))
        {
            hitRadiance = float3(0, 0, 0);
        }
    }
    // Off-screen hit: the previous-frame light-pass readback is the only
    // bounded-bounce source for the diffuse term. Out-of-frustum surfaces
    // contribute zero from this branch; the sky-NEE term below still
    // bootstraps energy independent of any prior-frame state.

    // TASK-6.10: add sky NEE at the secondary vertex. Closes the blue-channel
    // deficit that TASK-6.6's PT comparison surfaced — interior surfaces lit
    // mainly by sky-bounce-via-floor-and-walls saw zero sky contribution
    // here because the on-screen branch above bootstraps from the previous-
    // frame light-pass output (a feedback loop that converges from below)
    // and the off-screen branch reads the world-tile cache (which itself
    // is fed by these closest-hit invocations). Sky NEE adds an
    // independent radiance channel that does not depend on prior-frame
    // state, so the blue energy enters the cache on the very first frame.
    hitRadiance += SampleSkyNEE(hitPositionWS, g_Frame.frameIndex);

    payload.radiance = hitRadiance;
    payload.distance = RayTCurrent();
}
