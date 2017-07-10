#include "stdafx.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"
#include "StaticMeshComponent.h"

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

	addTestTriangle();

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//// normal coord attribute
	//glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	//glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void MeshData::update()
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
	std::vector<float> verticesBuffer(vertices.size() * 5);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[5 * i + 0] = vertices[i]->getPos().x;
		verticesBuffer[5 * i + 1] = vertices[i]->getPos().y;
		verticesBuffer[5 * i + 2] = vertices[i]->getPos().z;
		verticesBuffer[5 * i + 3] = vertices[i]->getTexCoord().x;
		verticesBuffer[5 * i + 4] = vertices[i]->getTexCoord().y;
		/*verticesBuffer[8 * i + 5] = vertices[i]->getNormal().x;
		verticesBuffer[8 * i + 6] = vertices[i]->getNormal().y;
		verticesBuffer[8 * i + 7] = vertices[i]->getNormal().z;*/
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);
}

void MeshData::addTestTriangle()
{
	VertexData l_VertexData1;
	l_VertexData1.addVertexData(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData2;
	l_VertexData2.addVertexData(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData3;
	l_VertexData3.addVertexData(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData4;
	l_VertexData4.addVertexData(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	m_vertices = { &l_VertexData1, &l_VertexData2, &l_VertexData3, &l_VertexData4 };
	m_intices = { 0, 1, 3, 1, 2 ,3};

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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void TextureData::shutdown()
{
}

void TextureData::addTextureData(int textureWidth, int textureHeight, unsigned char * textureData) const
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);
}

StaticMeshComponent::StaticMeshComponent()
{
}


StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::loadMesh(const std::string & meshFileName) const
{
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName) const
{
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(("../res/textures/" + textureFileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		m_textureData.addTextureData(width, height, data);
	}
	else
	{
		LogManager::printLog("Error: Failed to load texture: " + textureFileName);
	}
	stbi_image_free(data);
}

void StaticMeshComponent::render()
{
}

void StaticMeshComponent::init()
{
	m_textureData.init();
	loadTexture("container.jpg");
	m_meshData.init();
}

void StaticMeshComponent::update()
{
	getTransform()->update();
	m_textureData.update();
	m_meshData.update();
}

void StaticMeshComponent::shutdown()
{
	m_textureData.shutdown();
	m_meshData.shutdown();
}