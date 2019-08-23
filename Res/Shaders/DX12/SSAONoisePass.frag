// shadertype=hlsl
#include "common/common.hlsl"

Texture2D in_Position : register(t0);
Texture2D in_Normal : register(t1);
Texture2D in_randomRot : register(t2);

SamplerState SampleTypePoint : register(s0);
SamplerState SampleTypeWrap : register(s1);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 SSAOPassRT0 : SV_Target0;
};

static const float radius = 0.5f;
static const float bias = 0.05f;

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;
	float2 renderTargetSize;
	float level;
	in_Position.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);
	float2 texelSize = 1.0 / renderTargetSize;
	float2 screenTexCoords = input.position.xy * texelSize;

	float2 randomRotSize;
	in_randomRot.GetDimensions(0, randomRotSize.x, randomRotSize.y, level);

	float2 noiseScale = renderTargetSize / randomRotSize;
	// Repeat address mode
	float3 randomRot = in_randomRot.Sample(SampleTypeWrap, screenTexCoords * noiseScale).xyz;

	// alpha channel is used previously, remove its unwanted influence
	// world space position to view space
	float3 fragPos = in_Position.Sample(SampleTypePoint, screenTexCoords).xyz;
	float4 fragPosV4 = float4(fragPos, 1.0f);
	fragPosV4 = mul(fragPosV4, cameraCBuffer.t);
	fragPosV4 = mul(fragPosV4, cameraCBuffer.r);
	fragPos = fragPosV4.xyz;

	// world space normal to view space
	float3 normal = in_Normal.Sample(SampleTypePoint, screenTexCoords).xyz;
	float4 normalV4 = float4(normal, 0.0f);
	normalV4 = mul(normalV4, cameraCBuffer.r);
	normal = normalV4.xyz;
	normal = normalize(normal);

	// create TBN change-of-basis matrix: from tangent-space to view-space
	float3 tangent = normalize(randomRot - normal * dot(randomRot, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	// iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0f;
	for (int i = 0; i < 64; ++i)
	{
		// get sample position
		float3 randomHemisphereSampleDir = mul(SSAOKernels[i].xyz, TBN); // from tangent to view-space
		float3 randomHemisphereSamplePos = fragPos + randomHemisphereSampleDir * radius;

		// project sample position (to sample texture) (to get position on screen/texture)
		float4 randomFragSampleCoord = float4(randomHemisphereSamplePos, 1.0f);
		randomFragSampleCoord = mul(randomFragSampleCoord, cameraCBuffer.p_jittered); // from view to clip-space
		randomFragSampleCoord.xyz /= randomFragSampleCoord.w; // perspective divide
		randomFragSampleCoord.xyz = randomFragSampleCoord.xyz * 0.5f + 0.5f; // transform to range 0.0 - 1.0

		randomFragSampleCoord = saturate(randomFragSampleCoord);

		// Flip y
		randomFragSampleCoord.y = 1.0 - randomFragSampleCoord.y;

		// get sample depth
		float4 randomFragSamplePos = in_Position.Sample(SampleTypePoint, randomFragSampleCoord.xy);

		// alpha channel is used previously, remove its unwanted influence
		randomFragSamplePos.w = 1.0f;
		randomFragSamplePos = mul(randomFragSamplePos, cameraCBuffer.t);
		randomFragSamplePos = mul(randomFragSamplePos, cameraCBuffer.r);

		// range check & accumulate
		float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - randomFragSamplePos.z), 0.0001f));
		occlusion += (randomFragSamplePos.z > randomHemisphereSamplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / float(64));

	output.SSAOPassRT0 = float4(occlusion, occlusion, occlusion, 1.0);
	//output.SSAOPassRT0 = float4(randomRot, 1.0);
	return output;
}