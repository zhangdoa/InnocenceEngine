// shadertype=<glsl>
#version 450
#extension GL_ARB_shader_image_load_store : require
layout(location = 0) in vec3 TexCoord;
layout(location = 0) uniform uint uni_volumeDimension;

layout(binding = 0, rgba8) uniform volatile coherent image3D uni_voxelAlbedo;

void main()
{
	ivec3 outputCoord = ivec3(TexCoord);
	imageStore(uni_voxelAlbedo, outputCoord, vec4(TexCoord / uni_volumeDimension, 1));
};