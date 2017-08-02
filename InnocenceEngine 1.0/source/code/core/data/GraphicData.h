#pragma once
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLGraphicData.h"

class StaticMeshData
{
public:
	StaticMeshData();
	~StaticMeshData();

	void init();
	void update();
	void shutdown();
private:
	GLMeshData m_GLMeshData;
	std::vector<VertexData*> m_vertices;
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

	void addMeshData(std::vector<VertexData*>& vertices) const;
	void addTestSkybox();

private:
	GLuint m_VAO;
	GLuint m_VBO;

	std::vector<VertexData*> m_vertices;
};

class TextureData
{
public:
	TextureData();
	~TextureData();

	void init();
	void update();
	void shutdown();

	void loadTexture(const std::string& filePath) const;

private:
	GLTextureData m_GLTextureData;
};

class CubemapData
{
public:
	CubemapData();
	~CubemapData();

	void init();
	void update();
	void shutdown();

	void loadCubemap(const std::vector<std::string>& filePath) const;

private:
	GLCubemapData m_GLCubemapData;
};
