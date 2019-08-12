// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out vec2 thefrag_TexCoord;

void main()
{
	vec4 posWS = vec4(billboardUBO.data[gl_InstanceID].m[3][0], billboardUBO.data[gl_InstanceID].m[3][1], billboardUBO.data[gl_InstanceID].m[3][2], 1.0f);
	float distance = length(posWS - cameraUBO.globalPos);
	gl_Position = cameraUBO.p_original * cameraUBO.r * cameraUBO.t *  posWS;
	gl_Position /= gl_Position.w;
	float denom = distance;
	vec2 shearingRatio = vec2(1.0f / cameraUBO.WHRatio, 1.0) / clamp(denom, 1.0f, distance);
	gl_Position.xy += inPosition.xy * shearingRatio;
	thefrag_TexCoord = inTexCoord;
}