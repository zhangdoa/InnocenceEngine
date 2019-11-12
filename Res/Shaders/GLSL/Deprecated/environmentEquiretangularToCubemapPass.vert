// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec3 TexCoords;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_r;

void main()
{
	TexCoords = inPosition.xyz;
	gl_Position = uni_p * uni_r * inPosition;
}