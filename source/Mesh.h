#pragma once
#include "stdafx.h"
#include "Vertex.h"
#include "Util.h"

class Mesh
{
public:
	Mesh();
	Mesh(std::string fileName);
	Mesh(std::vector<Vertex*>& vertices, std::vector<unsigned int>& intices, bool calcNormals);
	~Mesh();
	void draw();

private:
	GLuint _vbo;
	GLuint _ibo;
	GLuint _size;
	void initMeshData();
	void addVertices(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices, bool calcNormals);
	Mesh* loadMesh(std::string fileName);
	void calcNormals(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices);
};

