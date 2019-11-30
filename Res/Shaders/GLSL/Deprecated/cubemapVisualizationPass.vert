// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec3 TexCoords;

void main()
{
	TexCoords = inPosition.xyz * -1.0;
	vec4 pos = perFrameCBuffer.data.p_original * perFrameCBuffer.data.r * -1.0 * inPosition;
	gl_Position = pos.xyww;
}