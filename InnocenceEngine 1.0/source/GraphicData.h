#pragma once
#include "LogManager.h"

class VertexData
{
public:
	VertexData();
	~VertexData();

	const glm::vec3& getPos() const;
	const glm::vec2& getTexCoord() const;
	const glm::vec3& getNormal() const;

	void setPos(const glm::vec3& pos);
	void setTexCoord(const glm::vec2& texCoord);
	void setNormal(const glm::vec3& normal);

	void addVertexData(const glm::vec3 & pos, const glm::vec2 & texCoord, const glm::vec3 & normal);
private:
	glm::vec3 m_pos;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
};

class MeshData
{
public:
	MeshData();
	~MeshData();

	void init();
	void update();
	void shutdown();

	void addMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const;
	void addTestCube();

private:
	GLuint m_VAO;
	GLuint m_VBO;
	GLuint m_IBO;

	std::vector<VertexData*> m_vertices;
	std::vector<unsigned int> m_intices;

};


class TextureData
{
public:
	TextureData();
	~TextureData();

	void init();
	void update();
	void shutdown();

	void loadTexture(const std::string& textureFileName) const;
	void addTextureData(int textureWidth, int textureHeight, unsigned char * textureData) const;
private:
	GLuint m_textureID;


};

class CubemapData
{
public:
	CubemapData();
	~CubemapData();

	void init();
	void update();
	void shutdown();

	void loadCubemap(const std::vector<std::string>& faceImagePath) const;
	void addCubemapData(size_t faceCount, int cubemapTextureWidth, int cubemapTextureHeight, unsigned char * cubemapTextureData) const;
private:
	GLuint m_cubemapTextureID;
};
