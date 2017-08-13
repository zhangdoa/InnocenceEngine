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
	void loadData(const std::string & meshFileName);

	void addTestCube();
	void addTestSkybox();

private:
	GLMeshData m_GLMeshData;
	std::vector<VertexData> m_vertices;
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
