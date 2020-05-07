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

void main(PixelInputType input)
{
	float3 N = normalize(input.Normal);

	float3 V = normalize(perFrameCBuffer.camera_posWS - input.posWS.xyz);
	float3 L = normalize(-perFrameCBuffer.sun_direction.xyz);
	float3 H = normalize(V + L);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float3 F0 = materialCBuffer.albedo.rgb;

	// Specular BRDF
	float roughness = materialCBuffer.MRAT.y;
	float f90 = 1.0;
	float3 F = fresnelSchlick(F0, f90, NdotV);
	float G = Unreal_GeometrySmith(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Frss = F * D * G / PI;

	// "Real-Time Rendering", 4th edition, pg. 624, "14.5.1 Coverage and Transmittance"
	float thickness = materialCBuffer.MRAT.w;
	float d = thickness / max(NdotV, eps);

	// transmittance luminance defined as "F0/albedo"
	float3 sigma = -(log(F0));

	float3 Tr = exp(-sigma * d);

	// surface radiance
	float3 Cs = Frss * materialCBuffer.albedo.rgb * perFrameCBuffer.sun_illuminance.xyz * NdotL;

	float4 RT0 = float4(Cs, materialCBuffer.albedo.a);

	// alpha channel as the mask
	float4 RT1 = float4(Tr, 1.0f);

	uint encodedColor = EncodeColor(RT0);
	uint depth = asuint(input.posCS.z);

	int2 writeCoord = (int2)input.posCS.xy;
	uint newPtr = in_atomicCounter.IncrementCounter();
	uint oldPtr;
	InterlockedExchange(out_headPtr[writeCoord], newPtr, oldPtr);

	uint4 item = uint4(oldPtr, encodedColor, depth, 0);
	out_transparentPassRT0[newPtr] = item;
}