// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_opaquePassRT0;
layout(location = 1) out vec4 uni_opaquePassRT1;
layout(location = 2) out vec4 uni_opaquePassRT2;

layout(location = 0) in GS_OUT
{
	vec4 posWS;
	vec2 texcoord;
	vec4 normal;
} fs_in;

layout(location = 0, binding = 0) uniform sampler2D uni_normalTexture;
layout(location = 1, binding = 1) uniform sampler2D uni_albedoTexture;
layout(location = 2, binding = 2) uniform sampler2D uni_metallicTexture;
layout(location = 3, binding = 3) uniform sampler2D uni_roughnessTexture;
layout(location = 4, binding = 4) uniform sampler2D uni_aoTexture;

void main()
{
	vec3 WorldSpaceNormal;
	vec3 albedo;
	float transparency = 1.0;
	vec3 MRA;

	if (materialUBO.useNormalTexture)
	{
		// get edge vectors of the pixel triangle
		vec3 dp1 = dFdx(fs_in.posWS.xyz);
		vec3 dp2 = dFdy(fs_in.posWS.xyz);
		vec2 duv1 = dFdx(fs_in.texcoord);
		vec2 duv2 = dFdy(fs_in.texcoord);

		// solve the linear system
		vec3 N = normalize(fs_in.normal.xyz);
		vec3 dp2perp = cross(dp2, N);
		vec3 dp1perp = cross(N, dp1);
		vec3 T = normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		vec3 B = normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		mat3 TBN = mat3(T, B, N);

		vec3 TangentSpaceNormal = texture(uni_normalTexture, fs_in.texcoord).rgb * 2.0 - 1.0;
		WorldSpaceNormal = normalize(TBN * TangentSpaceNormal);
	}
	else
	{
		WorldSpaceNormal = normalize(fs_in.normal.xyz);
	}

	if (materialUBO.useAlbedoTexture)
	{
		vec4 albedoTexture = texture(uni_albedoTexture, fs_in.texcoord);
		transparency = albedoTexture.a;
		if (transparency < 0.1)
		{
			discard;
		}
		albedo = albedoTexture.rgb;
	}
	else
	{
		albedo = materialUBO.Albedo.rgb;
	}

	if (materialUBO.useMetallicTexture)
	{
		MRA.r = texture(uni_metallicTexture, fs_in.texcoord).r;
	}
	else
	{
		MRA.r = materialUBO.MRAT.r;
	}

	if (materialUBO.useRoughnessTexture)
	{
		MRA.g = texture(uni_roughnessTexture, fs_in.texcoord).r;
	}
	else
	{
		MRA.g = materialUBO.MRAT.g;
	}

	if (materialUBO.useAOTexture)
	{
		MRA.b = texture(uni_aoTexture, fs_in.texcoord).r;
	}
	else
	{
		MRA.b = materialUBO.MRAT.b;
	}

	uni_opaquePassRT0 = vec4(fs_in.posWS.xyz, MRA.r);
	uni_opaquePassRT1 = vec4(WorldSpaceNormal, MRA.g);
	uni_opaquePassRT2 = vec4(albedo, MRA.b);
}