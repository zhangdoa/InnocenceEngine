// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) out vec4 out_textureValue;

layout(location = 0, binding = 3, rgba8) uniform readonly image3D uni_voxelTexture;

layout(location = 1) uniform uint uni_volumeDimension;

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