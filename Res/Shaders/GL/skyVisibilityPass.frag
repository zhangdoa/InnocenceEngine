// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_skyVisibility;
layout(location = 0) in vec3 TexCoords;

// ----------------------------------------------------------------------------
void main()
{
	uni_skyVisibility = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}