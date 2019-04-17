// shadertype=glsl
#version 450
layout(location = 0) out vec4 uni_geometryPassRT0;
layout(location = 1) out vec4 uni_geometryPassRT1;
layout(location = 2) out vec4 uni_geometryPassRT2;
layout(location = 3) out vec4 uni_geometryPassRT3;

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec4 thefrag_ClipSpacePos_current;
layout(location = 2) in vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) in vec2 thefrag_TexCoord;
layout(location = 4) in vec3 thefrag_Normal;
layout(location = 5) in float thefrag_UUID;

uniform sampler2D uni_normalTexture;
uniform sampler2D uni_albedoTexture;
uniform sampler2D uni_metallicTexture;
uniform sampler2D uni_roughnessTexture;
uniform sampler2D uni_aoTexture;

layout(std140) uniform materialUBO
{
	vec4 uni_albedo;
	vec4 uni_MRAT;
	bool uni_useNormalTexture;
	bool uni_useAlbedoTexture;
	bool uni_useMetallicTexture;
	bool uni_useRoughnessTexture;
	bool uni_useAOTexture;
};

void main()
{
	vec3 WorldSpaceNormal;
	vec3 albedo;
	float transparency = 1.0;
	vec3 MRA;

	if (uni_useNormalTexture)
	{
		// get edge vectors of the pixel triangle
		vec3 dp1 = dFdx(thefrag_WorldSpacePos.xyz);
		vec3 dp2 = dFdy(thefrag_WorldSpacePos.xyz);
		vec2 duv1 = dFdx(thefrag_TexCoord);
		vec2 duv2 = dFdy(thefrag_TexCoord);

		// solve the linear system
		vec3 N = normalize(thefrag_Normal);
		vec3 dp2perp = cross(dp2, N);
		vec3 dp1perp = cross(N, dp1);
		vec3 T = normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		vec3 B = normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		mat3 TBN = mat3(T, B, N);

		vec3 TangentSpaceNormal = texture(uni_normalTexture, thefrag_TexCoord).rgb * 2.0 - 1.0;
		WorldSpaceNormal = normalize(TBN * TangentSpaceNormal);
	}
	else
	{
		WorldSpaceNormal = normalize(thefrag_Normal);
	}

	if (uni_useAlbedoTexture)
	{
		vec4 albedoTexture = texture(uni_albedoTexture, thefrag_TexCoord);
		transparency = albedoTexture.a;
		if (transparency < 0.1)
		{
			discard;
		}
		albedo = albedoTexture.rgb;
	}
	else
	{
		albedo = uni_albedo.rgb;
	}

	if (uni_useMetallicTexture)
	{
		MRA.r = texture(uni_metallicTexture, thefrag_TexCoord).r;
	}
	else
	{
		MRA.r = uni_MRAT.r;
	}

	if (uni_useRoughnessTexture)
	{
		MRA.g = texture(uni_roughnessTexture, thefrag_TexCoord).r;
	}
	else
	{
		MRA.g = uni_MRAT.g;
	}

	if (uni_useAOTexture)
	{
		MRA.b = texture(uni_aoTexture, thefrag_TexCoord).r;
	}
	else
	{
		MRA.b = uni_MRAT.b;
	}

	uni_geometryPassRT0 = vec4(thefrag_WorldSpacePos.xyz, MRA.r);
	uni_geometryPassRT1 = vec4(WorldSpaceNormal, MRA.g);
	uni_geometryPassRT2 = vec4(albedo, MRA.b);
	vec4 motionVec = (thefrag_ClipSpacePos_current / thefrag_ClipSpacePos_current.w - thefrag_ClipSpacePos_previous / thefrag_ClipSpacePos_previous.w);
	uni_geometryPassRT3 = vec4(motionVec.xy * 0.5, thefrag_UUID, transparency);
}