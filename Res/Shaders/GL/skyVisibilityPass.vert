// shadertype=glsl
#include "common.glsl"
layout(location = 0) in vec3 in_Position;

layout(location = 0) out vec3 TexCoords;

void main()
{
	TexCoords = in_Position * -1.0;
	vec4 pos = uni_p_camera_original * uni_r_camera * -1.0 * vec4(in_Position, 1.0);
	gl_Position = pos.xyww;
}