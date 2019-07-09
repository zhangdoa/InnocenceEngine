// shadertype=<glsl>
#version 450
layout(location = 0) in vec3 in_Position;
layout(location = 0) uniform mat4 uni_m;

void main()
{
	gl_Position = uni_m * vec4(in_Position, 1.0);
}