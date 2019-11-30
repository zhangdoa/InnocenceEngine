// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec2 thefrag_TexCoord;

layout(std430, row_major, set = 0, binding = 12) buffer billboardCBufferBlock
{
	PerObject_CB data[];
} billboardCBuffer;

void main()
{
	vec4 posWS = vec4(billboardCBuffer.data[gl_InstanceIndex].m[3][0], billboardCBuffer.data[gl_InstanceIndex].m[3][1], billboardCBuffer.data[gl_InstanceIndex].m[3][2], 1.0f);
	float distance = length(posWS - perFrameCBuffer.data.camera_posWS);
	gl_Position = perFrameCBuffer.data.p_original * perFrameCBuffer.data.v *  posWS;
	gl_Position /= gl_Position.w;
	float denom = distance;
	vec2 shearingRatio = vec2(perFrameCBuffer.data.viewportSize.y / perFrameCBuffer.data.viewportSize.x, 1.0) / clamp(denom, 1.0f, distance);
	gl_Position.xy += inPosition.xy * shearingRatio;
	thefrag_TexCoord = inTexCoord;
}
