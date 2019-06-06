#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec2 outTexCoord;

void main() {
	gl_Position = inPosition;
	gl_Position.y = -gl_Position.y;
	outTexCoord = inTexCoord;
}