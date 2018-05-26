#include "BaseMesh.h"

void BaseMesh::setup()
{
	g_pLogSystem->printLog("BaseMesh: Warning: use the setup() with parameter!");
}

void BaseMesh::setup(meshType meshType, meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents)
{
	m_meshType = meshType;
	m_meshDrawMethod = meshDrawMethod;
	m_calculateNormals = calculateNormals;
	m_calculateTangents = calculateTangents;

	if (m_calculateNormals)
	{
		for (auto& l_vertices : m_vertices)
		{
			l_vertices.m_normal = l_vertices.m_pos.normalize();
		}
	}
}

void BaseMesh::initialize()
{
}

const objectStatus & BaseMesh::getStatus() const
{
	return m_objectStatus;
}

const meshID BaseMesh::getMeshID() const
{
	return m_meshID;
}

const vec4 BaseMesh::findMaxVertex() const
{
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
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
	return vec4(maxX, maxY, maxZ, 1.0);
}

const vec4 BaseMesh::findMinVertex() const
{
	double minX = 0;
	double minY = 0;
	double minZ = 0;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
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
	return vec4(minX, minY, minZ, 1.0);
}

void BaseMesh::addVertices(const Vertex & Vertex)
{
	m_vertices.emplace_back(Vertex);
}

void BaseMesh::addVertices(const vec4 & pos, const vec2 & texCoord, const vec4 & normal)
{
	m_vertices.emplace_back(Vertex(pos, texCoord, normal));
}

void BaseMesh::addVertices(double pos_x, double pos_y, double pos_z, double texCoord_x, double texCoord_y, double normal_x, double normal_y, double normal_z)
{
	m_vertices.emplace_back(Vertex(vec4(pos_x, pos_y, pos_z, 1.0), vec2(texCoord_x, texCoord_y), vec4(normal_x, normal_y, normal_z, 0.0)));
}

void BaseMesh::addIndices(unsigned int index)
{
	m_indices.emplace_back(index);
}

void BaseMesh::addUnitCube()
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


	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : m_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void BaseMesh::addUnitSphere()
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	double PI = 3.14159265359;

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			double xSegment = (double)x / (double)X_SEGMENTS;
			double ySegment = (double)y / (double)Y_SEGMENTS;
			double xPos = cos(xSegment * 2.0 * PI) * sin(ySegment * PI);
			double yPos = cos(ySegment * PI);
			double zPos = sin(xSegment * 2.0 * PI) * sin(ySegment * PI);

			Vertex l_VertexData;
			l_VertexData.m_pos = vec4(xPos, yPos, zPos, 1.0);
			l_VertexData.m_texCoord = vec2(xSegment, ySegment);
			l_VertexData.m_normal = vec4(xPos, yPos, zPos, 0.0).normalize();
			//l_VertexData.m_tangent = glm::normalize(glm::cross(glm::vec4(0.0, 0.0, 1.0), l_VertexData.m_normal));
			//l_VertexData.m_bitangent = glm::normalize(glm::cross(l_VertexData.m_tangent, l_VertexData.m_normal));
			m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
}

void BaseMesh::addUnitQuad()
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

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	m_indices = { 0, 1, 3, 1, 2, 3 };
}

void BaseMesh::addUnitLine()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(0.0f, 0.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2 };
	m_indices = { 0, 1 };
}
