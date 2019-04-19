#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 thefrag_WorldSpacePos;
layout(location = 1) in vec4 thefrag_ClipSpacePos_current;
layout(location = 2) in vec4 thefrag_ClipSpacePos_previous;
layout(location = 3) in vec2 thefrag_TexCoord;
layout(location = 4) in vec3 thefrag_Normal;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(thefrag_WorldSpacePos.xyz, 1.0);
}