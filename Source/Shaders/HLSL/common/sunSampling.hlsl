// shadertype=hlsl
//
// Cone-jitter sampler for the sun's angular disc. Single source of truth shared
// by PTRayGen.hlsl (offline reference) and SunShadowRTRayGen.hlsl
// (real-time RT shadows, TASK-138). Lifted out of PTRayGen.hlsl per
// "no copy-paste — extract shared logic" (safety-observability discipline).

#ifndef SUN_SAMPLING_HLSL
#define SUN_SAMPLING_HLSL

#include "common.hlsl"

// Cone-jitter `sunDir` inside the sun's angular cone. SUN_ANGULAR_RADIUS
// (common.hlsl) is the half-angle in radians — naming kept as-is (TASK-138
// design alignment §"Naming the constant"); rename across PT + the BSDF
// clamp + this header is a separate-CL change. xi is a Halton/blue-noise
// pair in [0,1]^2. Returns a unit-length perturbed direction.
//
// Convention parity with PT: identical formula to the original
// SampleSunDirection in PTRayGen.hlsl prior to extraction —
// PT-vs-rast shadow softness must match modulo TAA convergence.
float3 SampleSunDirection(float3 sunDir, float2 xi)
{
	float r = sin(SUN_ANGULAR_RADIUS);
	float d = cos(SUN_ANGULAR_RADIUS);

	float phi = TWO_PI * xi.x;
	float cosTheta = 1.0f - xi.y * (1.0f - d);
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	float3 up        = abs(sunDir.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangent   = normalize(cross(up, sunDir));
	float3 bitangent = cross(sunDir, tangent);

	return normalize(tangent * H.x + bitangent * H.y + sunDir * H.z);
}

#endif // SUN_SAMPLING_HLSL
