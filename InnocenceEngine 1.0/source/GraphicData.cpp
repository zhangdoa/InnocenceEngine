#include "stdafx.h"
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
	m_GLMeshData.update();
}

void StaticMeshData::shutdown()
{
	m_GLMeshData.shutdown();
}

SkyboxMeshData::SkyboxMeshData()
{
}

SkyboxMeshData::~SkyboxMeshData()
{
}

void SkyboxMeshData::init()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void SkyboxMeshData::update()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void SkyboxMeshData::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
}

void SkyboxMeshData::addMeshData(std::vector<VertexData*>& vertices) const
{
	std::vector<float> verticesBuffer(vertices.size() * 3);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[3 * i + 0] = vertices[i]->m_pos.x;
		verticesBuffer[3 * i + 1] = vertices[i]->m_pos.y;
		verticesBuffer[3 * i + 2] = vertices[i]->m_pos.z;
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);
}

void SkyboxMeshData::addTestSkybox()
{

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

