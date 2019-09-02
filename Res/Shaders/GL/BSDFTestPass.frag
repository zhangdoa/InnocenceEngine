// shadertype=glsl
#include "common/common.glsl"

layout(location = 0, binding = 0) uniform sampler2D uni_brdfLUT;
layout(location = 1, binding = 1) uniform sampler2D uni_brdfMSLUT;

layout(location = 0) out vec4 uni_BRDFTestPassRT0;

#include "common/BSDF.glsl"

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec4 thefrag_ClipSpacePos_current;
layout(location = 2) in vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) in vec2 thefrag_TexCoord;
layout(location = 4) in vec3 thefrag_Normal;
layout(location = 5) in float thefrag_UUID;

// ----------------------------------------------------------------------------
void main()
{
	vec3 V = normalize(cameraUBO.globalPos.xyz - thefrag_WorldSpacePos.xyz);
	vec3 L = V;
	vec3 H = normalize(V + L);
	vec3 N = thefrag_Normal;

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	float out_roughness = materialUBO.MRAT.g;
	float out_metallic = materialUBO.MRAT.r;
	vec3 F0 = vec3(0.04, 0.04, 0.04);
	vec3 out_albedo = materialUBO.Albedo.rgb;
	vec3 luminance = vec3(1.0, 1.0, 1.0);
	F0 = mix(F0, out_albedo, out_metallic);

	float F90 = 1.0;
	vec3 F = F_Schlick(F0, F90, LdotH);
	float G = V_SmithGGXCorrelated(NdotV, NdotL, out_roughness);
	float D = D_GGX(NdotH, out_roughness);
	vec3 Frss = F * G * D;

	vec3 Frms = getFrMS(uni_brdfLUT, uni_brdfMSLUT, NdotL, NdotV, F0, out_roughness);

	vec3 Fr = Frss + Frms;

	vec3 kS = F;
	vec3 kD = vec3(1.0, 1.0, 1.0) - kS;

	kD *= 1.0 - out_metallic;

	vec3 Fd = DisneyDiffuse2015(NdotV, NdotL, LdotH, out_roughness * out_roughness) * out_albedo;

	vec3 Lo = (kD * Fd + Fr);

	uni_BRDFTestPassRT0 = vec4(Lo, 1.0f);
}