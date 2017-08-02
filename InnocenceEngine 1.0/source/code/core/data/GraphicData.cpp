#include "../../main/stdafx.h"
#include "GraphicData.h"

StaticMeshData::StaticMeshData()
{
}


StaticMeshData::~StaticMeshData()
{
}

void StaticMeshData::init()
{
	m_GLMeshData.init();
}

void StaticMeshData::update()
{
	m_GLMeshData.update(m_intices);
}

void StaticMeshData::shutdown()
{
	m_GLMeshData.shutdown();
}

void StaticMeshData::addTestCube()
{
	VertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(0.5f, 0.5f, 0.5f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_1.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(0.5f, -0.5f, 0.5f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_2.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-0.5f, -0.5f, 0.5f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_3.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-0.5f, 0.5f, 0.5f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_4.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(0.5f, 0.5f, -0.5f);
	l_VertexData_5.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_5.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(0.5f, -0.5f, -0.5f);
	l_VertexData_6.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_6.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-0.5f, -0.5f, -0.5f);
	l_VertexData_7.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_7.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-0.5f, 0.5f, -0.5f);
	l_VertexData_8.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_8.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	m_vertices = { &l_VertexData_1, &l_VertexData_2, &l_VertexData_3, &l_VertexData_4, &l_VertexData_5, &l_VertexData_6, &l_VertexData_7, &l_VertexData_8 };

	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	m_GLMeshData.addGLMeshData(m_vertices, m_intices, false);
}

void StaticMeshData::addTestSkybox()
{
	VertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(1.0f, 1.0f, 1.0f);

	VertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(1.0f, -1.0f, 1.0f);

	VertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-1.0f, -1.0f, 1.0f);

	VertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-1.0f, 1.0f, 1.0f);

	VertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(1.0f, 1.0f, -1.0f);

	VertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(1.0f, -1.0f, -1.0f);

	VertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-1.0f, -1.0f, -1.0f);

	VertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-1.0f, 1.0f, -1.0f);

	m_vertices = { &l_VertexData_1, &l_VertexData_2, &l_VertexData_3, &l_VertexData_4, &l_VertexData_5, &l_VertexData_6, &l_VertexData_7, &l_VertexData_8 };

	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	m_GLMeshData.addGLMeshData(m_vertices, m_intices, false);
}

TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}

void TextureData::init()
{
	m_GLTextureData.init();
}

void TextureData::update()
{
	m_GLTextureData.update();
}

void TextureData::shutdown()
{
	m_GLTextureData.shutdown();
}

void TextureData::loadTexture(const std::string & filePath) const
{
	m_GLTextureData.loadTexture(filePath);
}

CubemapData::CubemapData()
{
}

CubemapData::~CubemapData()
{
}

void CubemapData::init()
{
	m_GLCubemapData.init();
}

void CubemapData::update()
{
	m_GLCubemapData.update();
}

void CubemapData::shutdown()
{
	m_GLCubemapData.shutdown();
}


void CubemapData::loadCubemap(const std::vector<std::string> & filePath) const
{
	m_GLCubemapData.loadCubemap(filePath);
}

