// shadertype=glsl
#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform float uni_voxelSize;
uniform vec3 uni_worldMinPoint;

uniform mat4 uni_p;
uniform mat4 uni_r;
uniform mat4 uni_t;
uniform mat4 uni_m;

in vec4 out_textureValue[];
out vec4 voxelColor;

void main()
{
	const vec4 cubeVertices[8] = vec4[8]
	(
		vec4(0.5f, 0.5f, 0.5f, 0.0f),
		vec4(0.5f, 0.5f, -0.5f, 0.0f),
		vec4(0.5f, -0.5f, 0.5f, 0.0f),
		vec4(0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f, 0.5f, 0.5f, 0.0f),
		vec4(-0.5f, 0.5f, -0.5f, 0.0f),
		vec4(-0.5f, -0.5f, 0.5f, 0.0f),
		vec4(-0.5f, -0.5f, -0.5f, 0.0f)
		);

	const int cubeIndices[24] = int[24]
	(
		0, 2, 1, 3, // right
		6, 4, 7, 5, // left
		5, 4, 1, 0, // up
		6, 7, 2, 3, // down
		4, 6, 0, 2, // front
		1, 3, 5, 7  // back
		);

	vec4 projectedVertices[8];

	for (int i = 0; i < 8; ++i)
	{
		vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
		projectedVertices[i] = uni_p * uni_r * uni_t * uni_m * vertex;
	}

	for (int face = 0; face < 6; ++face)
	{
		for (int vertex = 0; vertex < 4; ++vertex)
		{
			gl_Position = projectedVertices[cubeIndices[face * 4 + vertex]];

			voxelColor = out_textureValue[0];
			EmitVertex();
		}

		EndPrimitive();
	}
}