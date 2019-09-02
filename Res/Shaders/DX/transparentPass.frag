// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

struct PixelOutputType
{
	float4 transparentPassRT0 : COLOR0;
	float4 transparentPassRT1 : COLOR1;
};

#include "common/BSDF.hlsl"

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

float3 N = normalize(input.Normal);

float3 V = normalize(cameraCBuffer.globalPos.xyz - input.posWS.xyz);
float3 L = normalize(-sun_dir.xyz);
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
float3 Cs = Frss * sun_illuminance.xyz * materialCBuffer.albedo.rgb;

output.transparentPassRT0 = float4(Cs, materialCBuffer.albedo.a);
// alpha channel as the mask
output.transparentPassRT1 = float4((1.0f - materialCBuffer.albedo.a + Tr * materialCBuffer.albedo.a), 1.0f);

return output;
}