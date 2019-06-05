// shadertype=<glsl>
#version 450
layout(location = 0) out uint uni_earlyZPassRT0;

void main()
{
	uni_earlyZPassRT0 = floatBitsToUint(gl_FragCoord.z);
}