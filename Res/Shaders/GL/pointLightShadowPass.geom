// shadertype=glsl
#include "common.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 1) uniform mat4 uni_p;
layout(location = 2) uniform mat4 uni_r[6];
layout(location = 8) uniform mat4 uni_t;

layout(location = 0) out vec4 FragPos;

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		gl_Layer = face;
		for (int i = 0; i < 3; ++i)
		{
			FragPos = gl_in[i].gl_Position;
			gl_Position = uni_p * uni_r[face] * uni_t * FragPos;
			EmitVertex();
		}
		EndPrimitive();
	}
}