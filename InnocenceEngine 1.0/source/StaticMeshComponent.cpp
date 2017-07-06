#include "stdafx.h"
#include "StaticMeshComponent.h"

VertexData::VertexData()
{
}


VertexData::~VertexData()
{
}

const Vec3f& VertexData::getPos()
{
	return m_pos;
}

const Vec2f& VertexData::getTexCoord()
{
	return m_texCoord;
}

const Vec3f& VertexData::getNormal()
{
	return m_normal;
}

void VertexData::setPos(const Vec3f & pos)
{
	m_pos = pos;
}

void VertexData::setTexCoord(const Vec2f& texCoord)
{
	m_texCoord = texCoord;
}

void VertexData::setNormal(const Vec3f & normal)
{
	m_normal = normal;
}

void VertexData::addVertexData(const Vec3f & pos, const Vec2f & texCoord, const Vec3f & normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

MeshData::MeshData()
{
}


MeshData::~MeshData()
{
}

void MeshData::init()
{
	glGenVertexArrays(1, &m_vertexArrayID);
	glBindVertexArray(m_vertexArrayID);

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	// bindShader the VertexDataData Array Object first, then bindShader and set VertexData buffer(s), and then configure VertexData attributes(s).
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	addTestTriangle();
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void MeshData::update()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void MeshData::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
}


void MeshData::addMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals)
{
	if (calcNormals) {
		for (size_t i = 0; i < vertices.size(); i += 3) {
			int i0 = indices[i];
			int i1 = indices[i + 1];
			int i2 = indices[i + 2];

			Vec3f v1 = vertices[i1]->getPos() - vertices[i0]->getPos();
			Vec3f v2 = vertices[i2]->getPos() - vertices[i0]->getPos();

			Vec3f normal = v1.cross(v2).normalized();

			vertices[i0]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i1]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i2]->setNormal(vertices[i0]->getNormal() + (normal));

		}
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i]->setNormal(vertices[i]->getNormal().normalized());
		}
	}
	std::vector<float> verticesBuffer(vertices.size() * 3);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[3 * i + 0] = vertices[i]->getPos().getX();
		verticesBuffer[3 * i + 1] = vertices[i]->getPos().getY();
		verticesBuffer[3 * i + 2] = vertices[i]->getPos().getZ();
		//verticesBuffer[8 * i + 3] = vertices[i]->getTexCoord().getX();
		//verticesBuffer[8 * i + 4] = vertices[i]->getTexCoord().getY();
		//verticesBuffer[8 * i + 5] = vertices[i]->getNormal().getX();
		//verticesBuffer[8 * i + 6] = vertices[i]->getNormal().getY();
		//verticesBuffer[8 * i + 7] = vertices[i]->getNormal().getZ();
	}

	//std::vector<unsigned int> indicesBuffer = indices;
	//std::reverse(indicesBuffer.begin(), indicesBuffer.end());

	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * 4, &verticesBuffer[0], GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer.size(), indicesBuffer.data(), GL_STATIC_DRAW);
}

void MeshData::addTestTriangle()
{
	VertexData l_VertexData1;
	l_VertexData1.addVertexData(Vec3f(-1.0f, -1.0f, 1.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData2;
	l_VertexData2.addVertexData(Vec3f(1.0f, -1.0f, 0.5f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData3;
	l_VertexData3.addVertexData(Vec3f(0.0f, 1.0f, 0.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData4;
	l_VertexData4.addVertexData(Vec3f(0.0f, 1.0f, 0.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	m_vertices = { &l_VertexData1, &l_VertexData2, &l_VertexData3 };
	m_intices = { 1, 2, 3 };

	addMeshData(m_vertices, m_intices, false);
}


StaticMeshComponent::StaticMeshComponent()
{
}


StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::init()
{
	m_meshData.init();
}

void StaticMeshComponent::update()
{
	m_meshData.update();
}

void StaticMeshComponent::shutdown()
{
	m_meshData.shutdown();
}
