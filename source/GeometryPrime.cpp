#include "GeometryPrime.h"

Vertex::Vertex()
{
	_pos = Vec3f(0.0f, 0.0f, 0.0f);
	_texCoord = Vec2f(0.0f, 0.0f);
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos)
{
	_pos = pos;
	_texCoord = Vec2f(0.0f, 0.0f);
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos, Vec2f texCoord)
{
	_pos = pos;
	_texCoord = texCoord;
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos, Vec2f texCoord, Vec3f normal)
{
	_pos = pos;
	_texCoord = texCoord;
	_normal = normal;
}


Vertex::~Vertex()
{
}

const Vec3f& Vertex::getPos()
{
	return _pos;
}

const Vec2f& Vertex::getTexCoord()
{
	return _texCoord;
}

const Vec3f& Vertex::getNormal()
{
	return _normal;
}

void Vertex::setNormal(const Vec3f& normal)
{
	this->_normal = normal;
}

Mesh::Mesh()
{
	initMeshData();

	Vertex Vertex1 = Vertex(Vec3f(-1.0f, -1.0f, 0.0f));
	Vertex Vertex2 = Vertex(Vec3f(1.0f, -1.0f, 0.0f));
	Vertex Vertex3 = Vertex(Vec3f(0.0f, 1.0f, 0.0f));

	std::vector<Vertex*> vertices = { &Vertex1, &Vertex2, &Vertex3 };
	std::vector<unsigned int> intices = { 1, 2, 3 };

	addVertices(vertices, intices, false);

}

Mesh::Mesh(std::string fileName)
{
	initMeshData();
	loadMesh(fileName);
}

Mesh::Mesh(std::vector<Vertex*>& vertices, std::vector<unsigned int>& intices, bool calcNormals)
{
	initMeshData();
	addVertices(vertices, intices, calcNormals);
}


Mesh::~Mesh()
{
}

void Mesh::initMeshData()
{
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	_size = 0;
	glGenBuffers(1, &_vbo);
	//glGenBuffers(1, &_ibo);

}

void Mesh::addVertices(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices, bool calcNormals)
{
	if (calcNormals) {
		Mesh::calcNormals(vertices, indices);
	}
	_size = indices.size();

	//std::vector<float> verticesBuffer(vertices.size() * Vertex::SIZE);
	std::vector<float> verticesBuffer(vertices.size() * 3.0f);

	for (int i = 0; i < vertices.size(); i++) {
		verticesBuffer[3*i] = vertices[i]->getPos().getX();
		verticesBuffer[3*i+1] = vertices[i]->getPos().getY();
		verticesBuffer[3*i+2] = vertices[i]->getPos().getZ();
		//verticesBuffer.push_back(vertices[i]->getTexCoord().getX());
		//verticesBuffer.push_back(vertices[i]->getTexCoord().getY());
		//verticesBuffer.push_back(vertices[i]->getNormal().getX());
		//verticesBuffer.push_back(vertices[i]->getNormal().getY());
		//verticesBuffer.push_back(vertices[i]->getNormal().getZ());
	}

	//std::vector<unsigned int> indicesBuffer = indices;
	//std::reverse(indicesBuffer.begin(), indicesBuffer.end());

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size(), verticesBuffer.data(), GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer.size(), indicesBuffer.data(), GL_STATIC_DRAW);
}

void Mesh::draw()
{
	glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(1);
	//glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (void*)0);
	//glVertexAttribPointer(1, 2, GL_FLOAT, false, Vertex::SIZE * 4, (void*)12);
	//glVertexAttribPointer(2, 3, GL_FLOAT, false, Vertex::SIZE * 4, (void*)20);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	//glDrawElements(GL_TRIANGLES, _size, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	//glDisableVertexAttribArray(2);


}

Mesh * Mesh::loadMesh(std::string fileName)
{
	return nullptr;
}

void Mesh::calcNormals(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices)
{
	for (int i = 0; i < vertices.size(); i += 3) {
		int i0 = indices[i];
		int i1 = indices[i + 1];
		int i2 = indices[i + 2];

		Vec3f v1 = vertices[i1]->getPos() - vertices[i0]->getPos();
		Vec3f v2 = vertices[i2]->getPos() - vertices[i0]->getPos();

		Vec3f normal = v1.cross(v2).normalized();

		vertices[i0]->setNormal(vertices[i0]->getNormal()+(normal));
		vertices[i1]->setNormal(vertices[i0]->getNormal()+(normal));
		vertices[i2]->setNormal(vertices[i0]->getNormal()+(normal));

	}
	for (int i = 0; i < vertices.size(); i++)
	{
		vertices[i]->setNormal(vertices[i]->getNormal().normalized());
	}
}
