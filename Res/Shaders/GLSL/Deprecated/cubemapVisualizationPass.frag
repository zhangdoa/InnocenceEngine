// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_cubemapVisualization;
layout(location = 0) in vec3 TexCoords;

layout(location = 0, binding = 0) uniform samplerCube uni_cubemap;

// ----------------------------------------------------------------------------
void main()
{
	uni_cubemapVisualization = texture(uni_cubemap, TexCoords);
}