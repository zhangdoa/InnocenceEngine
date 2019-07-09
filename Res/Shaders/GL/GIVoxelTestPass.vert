// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

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