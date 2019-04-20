// shadertype=glsl
#version 450
#extension GL_ARB_shader_image_load_store : require

out vec4 out_textureValue;

layout(binding = 3, rgba8) uniform readonly image3D uni_voxelTexture;

uniform uint uni_volumeDimension;

void main()
{
	vec3 position = vec3
	(
		gl_VertexID % uni_volumeDimension,
		(gl_VertexID / uni_volumeDimension) % uni_volumeDimension,
		gl_VertexID / (uni_volumeDimension * uni_volumeDimension)
	);

	ivec3 texPos = ivec3(position);
	out_textureValue = imageLoad(uni_voxelTexture, texPos);

	gl_Position = vec4(position, 1.0f);
}