// shadertype=glsl
#include "common.glsl"

layout(location = 0) in TES_OUT
{
	vec3 positionWS;
	vec4 positionCS;
	vec4 positionCS_prev;
	vec2 texCoord;
	vec3 normal;
}fs_in;

layout(location = 0) out vec4 uni_terrainPassRT0;
layout(location = 1) out vec4 uni_terrainPassRT1;
layout(location = 2) out vec4 uni_terrainPassRT2;
layout(location = 3) out vec4 uni_terrainPassRT3;

layout(location = 2, binding = 2) uniform sampler2D uni_albedoTexture;
layout(location = 3, binding = 3) uniform sampler2D uni_metallicTexture;
layout(location = 4, binding = 4) uniform sampler2D uni_roughnessTexture;
layout(location = 5, binding = 5) uniform sampler2D uni_aoTexture;

void main()
{
	vec3 N = fs_in.normal.xyz;
	N = N * 2.0 - 1.0;
	N = normalize(N);

	vec3 albedo = texture(uni_albedoTexture, fs_in.texCoord).rgb;
	vec3 MRA;

	MRA.r = texture(uni_metallicTexture, fs_in.texCoord).r;
	MRA.g = texture(uni_roughnessTexture, fs_in.texCoord).r;
	MRA.b = texture(uni_aoTexture, fs_in.texCoord).r;

	uni_terrainPassRT0 = vec4(fs_in.positionWS, 0.5);
	uni_terrainPassRT1 = vec4(N, 0.5);
	uni_terrainPassRT2 = vec4(albedo, 1.0);
	vec3 motionVec = (fs_in.positionCS / fs_in.positionCS.w - fs_in.positionCS_prev / fs_in.positionCS_prev.w).xyz;

	uni_terrainPassRT3 = vec4(motionVec.xy * 0.5, 0.0, 1.0);
}