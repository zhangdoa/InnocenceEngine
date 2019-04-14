// shadertype=glsl
#version 450
layout(location = 0) out vec4 uni_terrainPassRT0;

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec2 thefrag_TexCoord;
layout(location = 2) in vec3 thefrag_Normal;

uniform sampler2D uni_albedoTexture;

void main()
{
	// let glsl calculate partial derivatives!
	vec3 Q1 = dFdx(thefrag_WorldSpacePos.xyz);
	vec3 Q2 = dFdy(thefrag_WorldSpacePos.xyz);
	vec2 st1 = dFdx(thefrag_TexCoord);
	vec2 st2 = dFdy(thefrag_TexCoord);

	vec3 N = vec3(0.0f, 0.0f, 1.0f);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	N = -normalize(cross(T, B));

	mat3 TBN = mat3(T, B, N);

	vec3 WorldSpaceNormal;

	WorldSpaceNormal = normalize(TBN * vec3(0.0f, 0.0f, 1.0f));

	vec3 albedo = texture(uni_albedoTexture, thefrag_TexCoord).rgb;
	albedo *= vec3(0.5f, 0.3f, 0.7f);

	uni_terrainPassRT0 = vec4(N, 1.0f);
}