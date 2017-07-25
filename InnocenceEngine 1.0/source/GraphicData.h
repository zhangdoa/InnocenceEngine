#pragma once
#include "LogManager.h"

union VertexData
{
	glm::vec3 m_pos;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
};

class StaticMeshVertexData
{
public:
	StaticMeshVertexData();
	~StaticMeshVertexData();

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

class SkyboxVertexData
{
public:
	SkyboxVertexData();
	~SkyboxVertexData();

	const glm::vec3& getPos() const;

	void setPos(const glm::vec3& pos);

	void addVertexData(const glm::vec3 & pos);

private:
	glm::vec3 m_pos;
};

class StaticMeshData
{
public:
	StaticMeshData();
	~StaticMeshData();

	void init();
	void update();
	void shutdown();

	void addMeshData(std::vector<StaticMeshVertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const;
	void addTestCube();

private:
	GLuint m_VAO;
	GLuint m_VBO;
	GLuint m_IBO;

	std::vector<StaticMeshVertexData*> m_vertices;
	std::vector<unsigned int> m_intices;
};

class SkyboxMeshData
{
public:
	SkyboxMeshData();
	~SkyboxMeshData();

	void init();
	void update();
	void shutdown();

	void addMeshData(std::vector<StaticMeshVertexData*>& vertices) const;
	void addTestSkybox();

private:
	GLuint m_VAO;
	GLuint m_VBO;

	std::vector<SkyboxVertexData*> m_vertices;

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
	void addCubemapData(unsigned int faceCount, int cubemapTextureWidth, int cubemapTextureHeight, unsigned char * cubemapTextureData) const;
private:
	GLuint m_cubemapTextureID;
};
