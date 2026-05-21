// shadertype=hlsl
#ifndef PT_DENOISE_SHARED_HLSL
#define PT_DENOISE_SHARED_HLSL

// Shared channel-layout and motion-vector convention for the screen-space
// PT denoiser. CL-1 lands the raygen-side write of these channels; future
// passes (CL-2 temporal accumulation, CL-3 spatial à-trous, CL-4
// composition) read them from the same texture set.
//
// Channel layout deliberately mirrors the rasterized GBuffer produced by
// opaqueGeometryProcessPass.frag so the existing DecodeGBuffer helper
// (common/lightPassCommon.hlsl) is reusable on PT-mode denoiser inputs
// without forking. The layout matches what DecodeGBuffer actually reads
// today (lightPassCommon.hlsl:55-66):
//
//   RT0 = float4(positionWS,             instanceID-or-0-for-sky)
//   RT1 = float4(normalWS,               metalness)
//   RT2 = float4(albedo,                 roughness)
//   RT3 = float4(motionVec.xy, hitDist,  0)
//
// Note RT1.a / RT2.a carry metalness / roughness respectively — same as
// the rasterizer GBuffer. CL-1 task design plan listed these alphas
// swapped (roughness in RT1, metalness in RT2); the HLSL here follows
// the actual rasterizer + DecodeGBuffer contract.
//
// Sky / no-hit pixels write RT0 = float4(0,0,0,0). DecodeGBuffer's
// l_RT0.a == 0 sky test (lightPassCommon.hlsl:55) is what makes
// instanceID == 0 (which maps to TLAS instance 0) collide with the sky
// flag at the decoder level. CL-1 lives with this because no consumer
// reads the channel yet — CL-2 history-rejection will add the
// disambiguating "non-zero RT0.a means surface" convention if needed
// (or a 1-based instanceID at write time).
//
// Motion-vector convention matches OpaquePass.frag:124:
//   motionVec_px = screenPos_prev - screenPos_curr        // pixels
// i.e. it points FROM the current pixel TO where it was last frame.
// SSRCTemporal.comp:184-194 reprojects with `previous_uv = uv + velocity`
// where `velocity = motionPx / viewportSize`; the denoiser passes added
// in CL-2 inherit the same sign and unit so the engine-side history
// reprojection helpers are reusable unchanged.

// Channel-name constants. Consumers should reference these instead of
// hard-coding swizzles, so a future layout change is grep-able.
//
// The position channel is RT0.rgb; the sky / instance discriminator is
// RT0.a (== 0 for sky/miss, > 0 otherwise — currently storing the raw
// InstanceID() which CL-2 may rebase to 1-indexed if instance 0 occurs).
#define PT_DENOISE_RT0_POSITION_RGB         /* float3 positionWS */
#define PT_DENOISE_RT0_INSTANCEID_W         /* float  instanceID, 0 == sky */

// Normal + metalness mirror lightPassCommon.hlsl:62-63.
#define PT_DENOISE_RT1_NORMAL_RGB           /* float3 normalWS    */
#define PT_DENOISE_RT1_METALNESS_W          /* float  metalness   */

// Albedo + roughness mirror lightPassCommon.hlsl:64-65.
#define PT_DENOISE_RT2_ALBEDO_RGB           /* float3 albedo      */
#define PT_DENOISE_RT2_ROUGHNESS_W          /* float  roughness   */

// Motion vector + hit distance. RT3.xy matches OpaquePass.frag:124's
// pixel-space convention; RT3.z carries the primary-hit distance (used
// by the CL-3 specular blur-radius modulation) and RT3.w is reserved
// for a future per-pixel sample-count or confidence channel.
#define PT_DENOISE_RT3_MOTIONVEC_XY         /* float2 (px_prev - px_curr) */
#define PT_DENOISE_RT3_HITDIST_Z            /* float  primary-hit distance */
#define PT_DENOISE_RT3_RESERVED_W           /* float  reserved             */

#endif // PT_DENOISE_SHARED_HLSL
