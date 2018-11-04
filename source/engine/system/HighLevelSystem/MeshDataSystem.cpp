#include "MeshDataSystem.h"

void MeshDataSystem::setup()
{
}

void MeshDataSystem::initialize()
{
}

void MeshDataSystem::update()
{
}

void MeshDataSystem::terminate()
{
}

vec4 MeshDataSystem::findMaxVertex(const MeshDataComponent& meshDataComponent)
{
	float maxX = 0;
	float maxY = 0;
	float maxZ = 0;

	std::for_each(meshDataComponent.m_vertices.begin(), meshDataComponent.m_vertices.end(), [&](Vertex val)
	{
		if (val.m_pos.x >= maxX)
		{
			maxX = val.m_pos.x;
		};

		if (val.m_pos.y >= maxY)
		{
			maxY = val.m_pos.y;
		};

		if (val.m_pos.z >= maxZ)
		{
			maxZ = val.m_pos.z;
		};
	});
	return vec4(maxX, maxY, maxZ, 1.0f);
}

vec4 MeshDataSystem::findMinVertex(const MeshDataComponent& meshDataComponent)
{
	float minX = 0.0f;
	float minY = 0.0f;
	float minZ = 0.0f;

	std::for_each(meshDataComponent.m_vertices.begin(), meshDataComponent.m_vertices.end(), [&](Vertex val)
	{
		if (val.m_pos.x <= minX)
		{
			minX = val.m_pos.x;
		};

		if (val.m_pos.y <= minY)
		{
			minY = val.m_pos.y;
		};

		if (val.m_pos.z <= minZ)
		{
			minZ = val.m_pos.z;
		};
	});
	return vec4(minX, minY, minZ, 1.0f);
}

void MeshDataSystem::addUnitCube(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);


	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : meshDataComponent.m_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	meshDataComponent.m_indices = { 0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7 ,6,
		4, 7, 0, 0, 7, 3,
		1, 2, 5, 5, 2, 6 };

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void MeshDataSystem::addUnitSphere(MeshDataComponent& meshDataComponent)
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	auto l_containerSize = X_SEGMENTS * Y_SEGMENTS;
	meshDataComponent.m_vertices.reserve(l_containerSize);

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = cos(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);
			float yPos = cos(ySegment * PI<float>);
			float zPos = sin(xSegment * 2.0f * PI<float>) * sin(ySegment * PI<float>);

			Vertex l_VertexData;
			l_VertexData.m_pos = vec4(xPos, yPos, zPos, 1.0f);
			l_VertexData.m_texCoord = vec2(xSegment, ySegment);
			l_VertexData.m_normal = vec4(xPos, yPos, zPos, 0.0f).normalize();
			//l_VertexData.m_tangent = glm::normalize(glm::cross(glm::vec4(0.0f, 0.0f, 1.0f), l_VertexData.m_normal));
			//l_VertexData.m_bitangent = glm::normalize(glm::cross(l_VertexData.m_tangent, l_VertexData.m_normal));
			meshDataComponent.m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				meshDataComponent.m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				meshDataComponent.m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				meshDataComponent.m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void MeshDataSystem::addUnitQuad(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	meshDataComponent.m_indices = { 0, 1, 3, 1, 2, 3 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}

void MeshDataSystem::addUnitLine(MeshDataComponent& meshDataComponent)
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(0.0f, 0.0f);

	meshDataComponent.m_vertices = { l_VertexData_1, l_VertexData_2 };
	meshDataComponent.m_indices = { 0, 1 };
	meshDataComponent.m_indicesSize = meshDataComponent.m_indices.size();
}
