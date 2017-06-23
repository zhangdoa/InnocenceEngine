#pragma once
#include "stdafx.h"
#include "Math.h"

class Vertex
{
public:
	Vertex();
	Vertex(Vec3f pos);
	Vertex(Vec3f pos, Vec2f texCoord);
	Vertex(Vec3f pos, Vec2f texCoord, Vec3f normal);
	~Vertex();

	static const int SIZE = 8;

	Vec3f* getPos();
	Vec2f* getTexCoord();
	Vec3f* getNormal();
	void setNormal(const Vec3f& normal);

private:
	Vec3f _pos;
	Vec2f _texCoord;
	Vec3f _normal;
};

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

