#include "stdafx.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"
#include "GraphicData.h"

VertexData::VertexData()
{
}


VertexData::~VertexData()
{
}

const glm::vec3& VertexData::getPos() const
{
	return m_pos;
}

const glm::vec2& VertexData::getTexCoord() const
{
	return m_texCoord;
}

const glm::vec3& VertexData::getNormal() const
{
	return m_normal;
}

void VertexData::setPos(const glm::vec3 & pos)
{
	m_pos = pos;
}

void VertexData::setTexCoord(const glm::vec2& texCoord)
{
	m_texCoord = texCoord;
}

void VertexData::setNormal(const glm::vec3 & normal)
{
	m_normal = normal;
}

void VertexData::addVertexData(const glm::vec3 & pos, const glm::vec2 & texCoord, const glm::vec3 & normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

MeshData::MeshData()
{
}


MeshData::~MeshData()
{
}

void MeshData::init()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	addTestCube();

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void MeshData::update()
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_intices.size(), GL_UNSIGNED_INT, 0);
	glDepthFunc(GL_LESS);
}

void MeshData::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);
}


void MeshData::addMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const
{
	if (calcNormals) {
		for (size_t i = 0; i < vertices.size(); i += 3) {
			int i0 = indices[i];
			int i1 = indices[i + 1];
			int i2 = indices[i + 2];

			glm::vec3 v1 = vertices[i1]->getPos() - vertices[i0]->getPos();
			glm::vec3 v2 = vertices[i2]->getPos() - vertices[i0]->getPos();

			glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

			vertices[i0]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i1]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i2]->setNormal(vertices[i0]->getNormal() + (normal));

		}
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i]->setNormal(glm::normalize(vertices[i]->getNormal()));
		}
	}
	std::vector<float> verticesBuffer(vertices.size() * 8);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[8 * i + 0] = vertices[i]->getPos().x;
		verticesBuffer[8 * i + 1] = vertices[i]->getPos().y;
		verticesBuffer[8 * i + 2] = vertices[i]->getPos().z;
		verticesBuffer[8 * i + 3] = vertices[i]->getTexCoord().x;
		verticesBuffer[8 * i + 4] = vertices[i]->getTexCoord().y;
		verticesBuffer[8 * i + 5] = vertices[i]->getNormal().x;
		verticesBuffer[8 * i + 6] = vertices[i]->getNormal().y;
		verticesBuffer[8 * i + 7] = vertices[i]->getNormal().z;
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);
}

void MeshData::addTestCube()
{
	VertexData l_VertexData_1;
	l_VertexData_1.addVertexData(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	VertexData l_VertexData_2;
	l_VertexData_2.addVertexData(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	VertexData l_VertexData_3;
	l_VertexData_3.addVertexData(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	VertexData l_VertexData_4;
	l_VertexData_4.addVertexData(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	VertexData l_VertexData_5;
	l_VertexData_5.addVertexData(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	VertexData l_VertexData_6;
	l_VertexData_6.addVertexData(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	VertexData l_VertexData_7;
	l_VertexData_7.addVertexData(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	VertexData l_VertexData_8;
	l_VertexData_8.addVertexData(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	m_vertices = { &l_VertexData_1, &l_VertexData_2, &l_VertexData_3, &l_VertexData_4, &l_VertexData_5, &l_VertexData_6, &l_VertexData_7, &l_VertexData_8 };
	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
	addMeshData(m_vertices, m_intices, false);
}


TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}

void TextureData::init()
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void TextureData::update()
{
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void TextureData::shutdown()
{
}

void TextureData::loadTexture(const std::string & textureFileName) const
{
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(("../res/textures/" + textureFileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		addTextureData(width, height, data);
	}
	else
	{
		LogManager::getInstance().printLog("Error: Failed to load texture: " + textureFileName);
	}
	stbi_image_free(data);
}

void TextureData::addTextureData(int textureWidth, int textureHeight, unsigned char * textureData) const
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);
}

CubemapData::CubemapData()
{
}

CubemapData::~CubemapData()
{
}

void CubemapData::init()
{
	glGenTextures(1, &m_cubemapTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTextureID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void CubemapData::update()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTextureID);
}

void CubemapData::shutdown()
{
}

void CubemapData::addCubemapData(size_t faceCount, int cubemapTextureWidth, int cubemapTextureHeight, unsigned char * cubemapTextureData) const
{
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceCount,
		0, GL_RGB, cubemapTextureWidth, cubemapTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, cubemapTextureData);
}

void CubemapData::loadCubemap(const std::vector<std::string> & faceImagePath) const
{
	int width, height, nrChannels;
	for (size_t i = 0; i < faceImagePath.size(); i++)
	{
		unsigned char *data = stbi_load( ("../res/textures/" + faceImagePath[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			addCubemapData(i, width, height, data);
		}
		else
		{
			LogManager::getInstance().printLog("Cubemap texture failed to load at path: " + ("../res/textures/" + faceImagePath[i]));
		}
		stbi_image_free(data);
	}
}