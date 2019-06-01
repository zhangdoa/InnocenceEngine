// shadertype=<glsl>
#version 450
#extension GL_ARB_shader_image_load_store : require

layout(location = 0) in GS_OUT
{
	vec3 wsPosition;
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
} gs_in;

layout(location = 0) out vec4 fragColor;
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(binding = 0, r32ui) uniform volatile coherent uimage3D uni_voxelAlbedo;
layout(binding = 1, r32ui) uniform volatile coherent uimage3D uni_voxelNormal;

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
	// Conservative Rasterization
	if (gs_in.position.x < gs_in.triangleAABB.x || gs_in.position.y < gs_in.triangleAABB.y ||
		gs_in.position.x > gs_in.triangleAABB.z || gs_in.position.y > gs_in.triangleAABB.w)
	{
		discard;
	}

	// writing coords position
	ivec3 position = ivec3(gs_in.wsPosition);

	// fragment albedo
	vec4 albedo = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// average albedo per fragments surrounding the voxel volume
	albedo.rgb *= 255.0;
	uint newVal = convVec4ToRGBA8(albedo);
	uint prevStoredVal = 0;
	uint curStoredVal;
	uint numIterations = 0;

	while ((curStoredVal = imageAtomicCompSwap(uni_voxelAlbedo, position, prevStoredVal, newVal))
		!= prevStoredVal
		&& numIterations < 255)
	{
		prevStoredVal = curStoredVal;
		vec4 rval = convRGBA8ToVec4(curStoredVal);
		rval.rgb = (rval.rgb * rval.a); // Denormalize
		vec4 curValF = rval + albedo;    // Add
		curValF.rgb /= curValF.a;       // Renormalize
		newVal = convVec4ToRGBA8(curValF);

		++numIterations;
	}
}