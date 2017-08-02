#include "../../../main/stdafx.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "../../third-party/stb_image.h"
#include "GLGraphicData.h"


GLGraphicData::GLGraphicData()
{
}


GLGraphicData::~GLGraphicData()
{
}


GLMeshData::GLMeshData()
{
}

GLMeshData::~GLMeshData()
{
}

void GLMeshData::init()
{
	generateData();
	addTestCube();
	attributeArray();
}

void GLMeshData::update()
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_intices.size(), GL_UNSIGNED_INT, 0);
}

void GLMeshData::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);
}

void GLMeshData::generateData()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);
}

void GLMeshData::attributeArray() const
{
	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLMeshData::addGLMeshData(std::vector<VertexData*>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const
{
	if (calcNormals) {
		for (size_t i = 0; i < vertices.size(); i += 3) {
			int i0 = indices[i];
			int i1 = indices[i + 1];
			int i2 = indices[i + 2];

			glm::vec3 v1 = vertices[i1]->m_pos - vertices[i0]->m_pos;
			glm::vec3 v2 = vertices[i2]->m_pos - vertices[i0]->m_pos;

			glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

			vertices[i0]->m_normal = vertices[i0]->m_normal + (normal);
			vertices[i1]->m_normal = vertices[i0]->m_normal + (normal);
			vertices[i2]->m_normal = vertices[i0]->m_normal + (normal);

		}

		// try some syntax candy
		std::for_each(vertices.begin(), vertices.end(), [](VertexData* val)
		{
			val->m_normal = glm::normalize(val->m_normal);
		});

		//for (size_t i = 0; i < vertices.size(); i++)
		//{
		//	vertices[i]->setNormal(glm::normalize(vertices[i]->m_normal));
		//}
	}
	std::vector<float> verticesBuffer(vertices.size() * 8);

	for (size_t i = 0; i < vertices.size(); i++)
	{
		verticesBuffer[8 * i + 0] = vertices[i]->m_pos.x;
		verticesBuffer[8 * i + 1] = vertices[i]->m_pos.y;
		verticesBuffer[8 * i + 2] = vertices[i]->m_pos.z;
		verticesBuffer[8 * i + 3] = vertices[i]->m_texCoord.x;
		verticesBuffer[8 * i + 4] = vertices[i]->m_texCoord.y;
		verticesBuffer[8 * i + 5] = vertices[i]->m_normal.x;
		verticesBuffer[8 * i + 6] = vertices[i]->m_normal.y;
		verticesBuffer[8 * i + 7] = vertices[i]->m_normal.z;
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);
}

void GLMeshData::addTestCube()
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

	addGLMeshData(m_vertices, m_intices, false);
}

GLTextureData::GLTextureData()
{
}

GLTextureData::~GLTextureData()
{
}

void GLTextureData::init()
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GLTextureData::update()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void GLTextureData::shutdown()
{
}

void GLTextureData::loadTexture(const std::string & textureFileName) const
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

void GLTextureData::addTextureData(int textureWidth, int textureHeight, unsigned char * GLTextureData) const
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, GLTextureData);
	glGenerateMipmap(GL_TEXTURE_2D);
}

GLCubemapData::GLCubemapData()
{
}

GLCubemapData::~GLCubemapData()
{
}

void GLCubemapData::init()
{
	glGenTextures(1, &m_cubemapTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTextureID);

	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

void GLCubemapData::update()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTextureID);
}

void GLCubemapData::shutdown()
{
}

void GLCubemapData::addCubemapData(unsigned int faceCount, int cubemapTextureWidth, int cubemapTextureHeight, unsigned char * cubemapGLTextureData) const
{
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceCount,
		0, GL_RGB, cubemapTextureWidth, cubemapTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, cubemapGLTextureData);
}

void GLCubemapData::loadCubemap(const std::vector<std::string> & faceImagePath) const
{
	int width, height, nrChannels;
	for (unsigned int i = 0; i < faceImagePath.size(); i++)
	{
		unsigned char *data = stbi_load(("../res/textures/" + faceImagePath[i]).c_str(), &width, &height, &nrChannels, 0);
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
