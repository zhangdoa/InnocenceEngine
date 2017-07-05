#pragma once
#include "IGameEntity.h"

class VertexData
{
public:
	VertexData();
	~VertexData();

	const Vec3f& getPos();
	const Vec2f& getTexCoord();
	const Vec3f& getNormal();

	void setPos(const Vec3f& pos);
	void setTexCoord(const Vec2f& texCoord);
	void setNormal(const Vec3f& normal);

	void addVertexData(const Vec3f & pos, const Vec2f & texCoord, const Vec3f & normal);
private:
	Vec3f m_pos;
	Vec2f m_texCoord;
	Vec3f m_normal;
};

class MeshData
{
public:
	MeshData();
	~MeshData();

	void init();
	void update();
	void shutdown();

	void addMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals);
	void addTestTriangle();

private:
	GLuint m_vertexArrayID;
	GLuint m_VBO;
	GLuint m_VAO;
	GLuint m_IBO;

	std::vector<VertexData*> m_vertices;
	std::vector<unsigned int> m_intices;

};

class StaticMeshComponent : public IGameEntity
{
public:
	StaticMeshComponent();
	~StaticMeshComponent();

private:
	MeshData m_meshData;
	void init() override;
	void update() override;
	void shutdown() override;
};

