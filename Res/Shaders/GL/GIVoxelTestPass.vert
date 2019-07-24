// shadertype=glsl
#include "common.glsl"

layout(location = 0) uniform uint uni_volumeDimension;
layout(location = 0) out vec3 TexCoord;

void main()
{
	vec3 position = vec3
	(
		gl_VertexID % uni_volumeDimension,
		(gl_VertexID / uni_volumeDimension) % uni_volumeDimension,
		gl_VertexID / (uni_volumeDimension * uni_volumeDimension)
	);
	TexCoord = position;
	gl_Position = vec4(position / float(uni_volumeDimension), 1.0f);
}