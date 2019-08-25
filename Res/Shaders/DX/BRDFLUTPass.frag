// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 brdfLUT : SV_Target0;
};

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway floattor
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space H floattor to world-space sample floattor
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
float D_GGX(float NdotH, float roughness)
{
	float a = roughness;
	float a2 = a * a;
	float f = (NdotH * a2 - NdotH) * NdotH + 1;
	return a2 / pow(f, 2.0);
}
// ----------------------------------------------------------------------------
float G_SchlickGGX(float NdotV, float roughness)
{
	// note that we use a different k for IBL
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = G_SchlickGGX(NdotV, roughness);
	float ggx1 = G_SchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
	float alphaG2 = alphaG * alphaG;
	float Lambda_GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaG2) + alphaG2);
	float Lambda_GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaG2) + alphaG2);
	return 0.5 / max((Lambda_GGXV + Lambda_GGXL), 0.00001);
}
// ----------------------------------------------------------------------------
float4 IntegrateBRDF(float NdotV, float roughness)
{
	float3 V;
	V.x = sqrt(1.0 - NdotV * NdotV);
	V.y = 0.0;
	V.z = NdotV;

	float A = 0.0;
	float B = 0.0;
	float RsF1 = 0.0;

	float3 N = float3(0.0, 0.0, 1.0);

	const uint SAMPLE_COUNT = 1024u;
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		// generates a sample floattor that's biased towards the
		// preferred alignment direction (importance sampling).
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = saturate(L.z);
		float NdotH = saturate(H.z);
		float VdotH = saturate(dot(V, H));

		if (NdotL > 0.0)
		{
			float G = G_Smith(N, V, L, roughness);
			float V = G * VdotH / (NdotH * NdotV);
			float D = D_GGX(NdotH, roughness);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * V;
			B += Fc * V;
			RsF1 += V * NdotL;
		}
	}

	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);
	RsF1 /= float(SAMPLE_COUNT);
	return float4(A, B, RsF1, 1.0);
}
// ----------------------------------------------------------------------------
PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	output.brdfLUT = IntegrateBRDF(input.texcoord.x, input.texcoord.y);
	return output;
}