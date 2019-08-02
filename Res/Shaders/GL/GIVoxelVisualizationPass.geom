// shadertype=glsl
#include "common/common.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 36) out;

layout(location = 2) uniform float uni_voxelSize;
layout(location = 3) uniform vec4 uni_worldMinPoint;

layout(location = 4) uniform mat4 uni_p;
layout(location = 5) uniform mat4 uni_r;
layout(location = 6) uniform mat4 uni_t;
layout(location = 7) uniform mat4 uni_m_local;

layout(location = 0) in vec4 out_textureValue[];
layout(location = 0) out vec4 voxelColor;

void main()
{
	const vec4 cubeVertices[8] = vec4[8]
	(
		vec4(0.5f, 0.5f, 0.5f, 0.0f),
		vec4(0.5f, -0.5f, 0.5f, 0.0f),
		vec4(-0.5f, -0.5f, 0.5f, 0.0f),
		vec4(-0.5f, 0.5f, 0.5f, 0.0f),
		vec4(0.5f, 0.5f, -0.5f, 0.0f),
		vec4(0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f, 0.5f, -0.5f, 0.0f)
		);

	const int cubeIndices[36] = int[36]
	(
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		7, 0, 4, 0, 7, 3,
		1, 2, 5, 5, 2, 6
		);

	vec4 projectedVertices[8];

	for (int i = 0; i < 8; ++i)
	{
		vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
		projectedVertices[i] = uni_p * uni_r * uni_t * uni_m_local * vertex;
	}

	for (int triangle = 0; triangle < 12; ++triangle)
	{
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			gl_Position = projectedVertices[cubeIndices[triangle * 3 + vertex]];

			voxelColor = out_textureValue[0];
			EmitVertex();
		}

		EndPrimitive();
	}
}