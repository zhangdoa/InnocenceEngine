// shadertype=glsl
#include "common/common.glsl"
#include "common/BRDF.glsl"

layout(location = 0, index = 0) out vec4 uni_transparentPassRT0;
layout(location = 0, index = 1) out vec4 uni_transparentPassRT1;

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec2 thefrag_TexCoord;
layout(location = 2) in vec3 thefrag_Normal;

void main()
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

	vec3 WorldSpaceNormal;
	WorldSpaceNormal = normalize(TBN * vec3(0.0f, 0.0f, 1.0f));

	N = WorldSpaceNormal;
	vec3 V = normalize(cameraUBO.globalPos.xyz - thefrag_WorldSpacePos.xyz);
	vec3 L = normalize(-sunUBO.data.direction.xyz);
	vec3 H = normalize(V + L);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	vec3 F0 = materialUBO.Albedo.rgb;

	// Specular BRDF
	float roughness = materialUBO.MRAT.y;
	float f90 = 1.0;
	vec3 F = fr_F_Schlick(F0, f90, NdotV);
	float G = Unreal_GeometrySmith(NdotV, NdotL, roughness);
	float D = fr_D_GGX(NdotH, roughness);
	vec3 Frss = F * D * G / PI;

	// "Real-Time Rendering", 4th edition, pg. 624, "14.5.1 Coverage and Transmittance"
	float thickness = materialUBO.MRAT.w;
	float d = thickness / max(NdotV, eps);

	// transmittance luminance defined as "F0/albedo"
	vec3 sigma = -(log(F0));

	vec3 Tr = exp(-sigma * d);

	// surface radiance
	vec3 Cs = Frss * sunUBO.data.luminance.xyz * materialUBO.Albedo.rgb;

	uni_transparentPassRT0 = vec4(Cs, materialUBO.Albedo.a);
	// alpha channel as the mask
	uni_transparentPassRT1 = vec4((1.0f - materialUBO.Albedo.a + Tr * materialUBO.Albedo.a), 1.0f);
}