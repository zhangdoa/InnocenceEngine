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

const Vec3f& VertexData::getPos()
{
	return m_pos;
}

const Vec2f& VertexData::getTexCoord()
{
	return m_texCoord;
}

const Vec3f& VertexData::getNormal()
{
	return m_normal;
}

void VertexData::setPos(const Vec3f & pos)
{
	m_pos = pos;
}

void VertexData::setTexCoord(const Vec2f& texCoord)
{
	m_texCoord = texCoord;
}

void VertexData::setNormal(const Vec3f & normal)
{
	m_normal = normal;
}

void VertexData::addVertexData(const Vec3f & pos, const Vec2f & texCoord, const Vec3f & normal)
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// normal coord attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);
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


void MeshData::addMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals)
{
	if (calcNormals) {
		for (size_t i = 0; i < vertices.size(); i += 3) {
			int i0 = indices[i];
			int i1 = indices[i + 1];
			int i2 = indices[i + 2];

			Vec3f v1 = vertices[i1]->getPos() - vertices[i0]->getPos();
			Vec3f v2 = vertices[i2]->getPos() - vertices[i0]->getPos();

			Vec3f normal = v1.cross(v2).getNormalizedVec3f();

			vertices[i0]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i1]->setNormal(vertices[i0]->getNormal() + (normal));
			vertices[i2]->setNormal(vertices[i0]->getNormal() + (normal));

		}
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i]->setNormal(vertices[i]->getNormal().getNormalizedVec3f());
		}
	}
	std::vector<float> verticesBuffer(vertices.size() * 8);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[8 * i + 0] = vertices[i]->getPos().getX();
		verticesBuffer[8 * i + 1] = vertices[i]->getPos().getY();
		verticesBuffer[8 * i + 2] = vertices[i]->getPos().getZ();
		verticesBuffer[8 * i + 3] = vertices[i]->getTexCoord().getX();
		verticesBuffer[8 * i + 4] = vertices[i]->getTexCoord().getY();
		verticesBuffer[8 * i + 5] = vertices[i]->getNormal().getX();
		verticesBuffer[8 * i + 6] = vertices[i]->getNormal().getY();
		verticesBuffer[8 * i + 7] = vertices[i]->getNormal().getZ();
	}

	std::vector<unsigned int> indicesBuffer = indices;
	std::reverse(indicesBuffer.begin(), indicesBuffer.end());

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * 4, &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer.size() * 4, &indicesBuffer[0], GL_STATIC_DRAW);
}

void MeshData::addTestTriangle()
{
	VertexData l_VertexData1;
	l_VertexData1.addVertexData(Vec3f(0.5f, 0.5f, 0.0f), Vec2f(1.0f, 1.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData2;
	l_VertexData2.addVertexData(Vec3f(0.5f, -0.5f, 0.0f), Vec2f(1.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData3;
	l_VertexData3.addVertexData(Vec3f(-0.5f, -0.5f, 0.0f), Vec2f(0.0, 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

	VertexData l_VertexData4;
	l_VertexData4.addVertexData(Vec3f(-0.5f, 0.5f, 0.0f), Vec2f(0.0, 1.0f), Vec3f(0.0f, 0.0f, 0.0f));

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

void TextureData::addTextureData(int textureWidth, int textureHeight, unsigned char * textureData)
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

void StaticMeshComponent::loadMesh(const std::string & meshFileName)
{
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName)
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
	loadTexture("test.png");
	m_meshData.init();
}

void StaticMeshComponent::update()
{
	m_textureData.update();
	m_meshData.update();
}

void StaticMeshComponent::shutdown()
{
	m_textureData.shutdown();
	m_meshData.shutdown();
}