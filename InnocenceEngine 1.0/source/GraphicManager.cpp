#include "stdafx.h"
#include "GraphicManager.h"

using namespace nmsp_GraphicManager;

Vertex::Vertex()
{
}


Vertex::~Vertex()
{
}

const Vec3f& Vertex::getPos()
{
	return m_pos;
}

const Vec2f& Vertex::getTexCoord()
{
	return m_texCoord;
}

const Vec3f& Vertex::getNormal()
{
	return m_normal;
}

void Vertex::setPos(const Vec3f & pos)
{
	m_pos = pos;
}

void Vertex::setTexCoord(const Vec2f& texCoord)
{
	m_texCoord = texCoord;
}

void Vertex::setNormal(const Vec3f & normal)
{
	m_normal = normal;
}

void Vertex::addVertexData(Vec3f pos, Vec2f texCoord, Vec3f normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

Mesh::Mesh()
{
}


Mesh::~Mesh()
{
}

void Mesh::init()
{

	addMeshData(m_vertices, m_intices, false);

	glGenVertexArrays(1, &m_vertexArrayID);
	glBindVertexArray(m_vertexArrayID);

	// An array of 3 vectors which represents 3 vertices
	/*GLfloat g_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};*/

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	addTestTriangle();
	//glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::update()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Mesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
}


void Mesh::addMeshData(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices, bool calcNormals)
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
	std::vector<GLfloat> verticesBuffer(vertices.size() * 3);

	for (size_t i = 0; i < vertices.size(); i++) {
		verticesBuffer[3 * i] = vertices[i]->getPos().getX();
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

	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * 4, &verticesBuffer, GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer.size(), indicesBuffer.data(), GL_STATIC_DRAW);
}

void Mesh::addTestTriangle()
{
	Vertex l_vertex1;
	l_vertex1.addVertexData(Vec3f(-1.0f, -1.0f, 0.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	Vertex l_vertex2;
	l_vertex1.addVertexData(Vec3f(1.0f, -1.0f, 0.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	Vertex l_vertex3;
	l_vertex1.addVertexData(Vec3f(0.0f, 1.0f, 0.0f), Vec2f(0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	m_vertices = { &l_vertex1, &l_vertex2, &l_vertex3 };
	m_intices = { 1, 2, 3 };
}

GraphicManager::GraphicManager()
{
}


GraphicManager::~GraphicManager()
{
}


void GraphicManager::init()
{
	m_uiManager.exec(INIT);
	m_renderingManager.exec(INIT);
	testTriangle.init();
	this->setStatus(INITIALIZIED);
	printLog("GraphicManager has been initialized.");
}

void GraphicManager::update()
{
	if (m_uiManager.getStatus() == INITIALIZIED)
	{
		m_uiManager.exec(UPDATE);
		m_renderingManager.exec(UPDATE);
		testTriangle.update();
	}
	else
	{
		this->setStatus(STANDBY);
		printLog("GraphicManager is stand-by.");
	}
}

void GraphicManager::shutdown()
{
	m_renderingManager.exec(SHUTDOWN);
	m_uiManager.exec(SHUTDOWN);
	testTriangle.shutdown();
	this->setStatus(UNINITIALIZIED);
	printLog("GraphicManager has been shutdown.");
}