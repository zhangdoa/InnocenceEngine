// shadertype=glsl
#include "common.glsl"

layout(location = 0) in GS_OUT
{
	vec3 outputCoord;
	vec3 positionLS;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
} gs_in;

layout(binding = 0, rgba8) uniform volatile coherent image3D uni_voxelAlbedo;
layout(binding = 0) uniform sampler2D uni_albedoTexture;

vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)),
		float((val & 0x0000FF00) >> 8U),
		float((val & 0x00FF0000) >> 16U),
		float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U |
		(uint(val.z) & 0x000000FF) << 16U |
		(uint(val.y) & 0x000000FF) << 8U |
		(uint(val.x) & 0x000000FF);
}

void main()
{
	if (gs_in.positionLS.x < gs_in.triangleAABB.x || gs_in.positionLS.y < gs_in.triangleAABB.y ||
		gs_in.positionLS.x > gs_in.triangleAABB.z || gs_in.positionLS.y > gs_in.triangleAABB.w)
	{
		discard;
	}

	// fragment albedo
	vec4 albedo = vec4(1.0f);
	if (uni_useAlbedoTexture)
	{
		vec4 albedoTexture = texture(uni_albedoTexture, gs_in.texCoord);
		albedo.rgb = albedoTexture.rgb;
	}
	else
	{
		albedo.rgb = uni_albedo.rgb;
	}

	// writing coords positionLS
	ivec3 outputCoord = ivec3(gs_in.outputCoord);
	imageStore(uni_voxelAlbedo, outputCoord, albedo);

	//// average albedo per fragments surrounding the voxel volume
	//albedo.rgb *= 255.0;
	//uint newVal = convVec4ToRGBA8(albedo);
	//uint prevStoredVal = 0;
	//uint curStoredVal;
	//uint numIterations = 0;

	//while ((curStoredVal = imageAtomicCompSwap(uni_voxelAlbedo, gs_in.positionLS, prevStoredVal, newVal))
	//	!= prevStoredVal
	//	&& numIterations < 255)
	//{
	//	prevStoredVal = curStoredVal;
	//	vec4 rval = convRGBA8ToVec4(curStoredVal);
	//	rval.rgb = (rval.rgb * rval.a); // Denormalize
	//	vec4 curValF = rval + albedo;    // Add
	//	curValF.rgb /= curValF.a;       // Renormalize
	//	newVal = convVec4ToRGBA8(curValF);

	//	++numIterations;
	//}
}