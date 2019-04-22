#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec4 thefrag_ClipSpacePos_current;
layout(location = 2) in vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) in vec2 thefrag_TexCoord;
layout(location = 4) in vec3 thefrag_Normal;
layout(location = 5) in float thefrag_UUID;

layout(location = 0) out vec4 opaquePassRT0;
layout(location = 1) out vec4 opaquePassRT1;
layout(location = 2) out vec4 opaquePassRT2;
layout(location = 3) out vec4 opaquePassRT3;

layout(std140, set = 0, binding = 2) uniform materialUBO
{
	vec4 uni_albedo;
	vec4 uni_MRAT;
	bool uni_useNormalTexture;
	bool uni_useAlbedoTexture;
	bool uni_useMetallicTexture;
	bool uni_useRoughnessTexture;
	bool uni_useAOTexture;
};

void main() {
	vec3 WorldSpaceNormal = normalize(thefrag_Normal);
	opaquePassRT0 = vec4(thefrag_WorldSpacePos.xyz, uni_MRAT.x);
	opaquePassRT1 = vec4(WorldSpaceNormal, uni_MRAT.y);
	opaquePassRT2 = vec4(uni_albedo.xyz, uni_MRAT.z);
	vec4 motionVec = (thefrag_ClipSpacePos_current / thefrag_ClipSpacePos_current.w - thefrag_ClipSpacePos_previous / thefrag_ClipSpacePos_previous.w);
	opaquePassRT3 = vec4(motionVec.xy * 0.5, thefrag_UUID, uni_albedo.w);
}