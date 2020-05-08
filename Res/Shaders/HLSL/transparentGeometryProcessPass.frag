// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

RWStructuredBuffer<uint> in_atomicCounter : register(u0);
RWTexture2D<uint> out_headPtr : register(u1);
RWStructuredBuffer<uint4> out_transparentPassRT0 : register(u2);
RWStructuredBuffer<uint4> out_transparentPassRT1 : register(u3);

#include "common/BSDF.hlsl"

[earlydepthstencil]
void main(PixelInputType input)
{
	float3 N = normalize(input.Normal);

	float3 V = normalize(perFrameCBuffer.camera_posWS - input.posWS.xyz);
	float3 L = normalize(-perFrameCBuffer.sun_direction.xyz);
	float3 H = normalize(V + L);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float LdotH = max(dot(L, H), 0.0);

	float3 F0 = materialCBuffer.albedo.rgb;
	float F90 = 1.0;
	float metallic = materialCBuffer.MRAT.x;
	float roughness = materialCBuffer.MRAT.y;

	// Specular BRDF
	float3 FresnelFactor = fresnelSchlick(F0, F90, NdotV);
	float G = Unreal_GeometrySmith(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Frss = FresnelFactor * D * G / PI;

	// "Real-Time Rendering", 4th edition, pg. 624, "14.5.1 Coverage and Transmittance"
	float thickness = materialCBuffer.MRAT.w;
	float d = thickness / max(NdotV, eps);

	// transmittance luminance defined as "F0/albedo"
	float3 sigma = -(log(F0));

	float3 Tr = exp(-sigma * d);

	// surface radiance
	float3 Cs = Frss * perFrameCBuffer.sun_illuminance.xyz * NdotL * (1.0 - materialCBuffer.MRAT.z);

	float4 RT0 = float4(Cs, 0.0);

	uint encodedRT0 = EncodeColor(RT0);
	uint4 encodedRT1 = uint4(asuint(Tr.x), asuint(Tr.y), asuint(Tr.z), asuint(materialCBuffer.albedo.a));
	uint depth = asuint(input.posCS.z);

	int2 writeCoord = (int2)input.posCS.xy;
	uint newPtr = in_atomicCounter.IncrementCounter();

	uint oldPtr;
	InterlockedExchange(out_headPtr[writeCoord], newPtr, oldPtr);

	uint4 itemRT0 = uint4(oldPtr, encodedRT0, depth, 0);

	out_transparentPassRT0[newPtr] = itemRT0;
	out_transparentPassRT1[newPtr] = encodedRT1;
}