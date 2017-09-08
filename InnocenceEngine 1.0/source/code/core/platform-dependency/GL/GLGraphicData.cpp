#include "../../../main/stdafx.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "../../third-party/stb_image.h"
#include "GLGraphicData.h"

GLMeshData::GLMeshData()
{
}

GLMeshData::~GLMeshData()
{
}

void GLMeshData::init()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);
}

void GLMeshData::update(std::vector<unsigned int>& indices)
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void GLMeshData::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);
}

void GLMeshData::sendDataToGPU(std::vector<GLVertexData>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const
{
	if (calcNormals) {
		for (size_t i = 0; i < vertices.size(); i += 3) {
			int i0 = indices[i];
			int i1 = indices[i + 1];
			int i2 = indices[i + 2];

			glm::vec3 v1 = vertices[i1].m_pos - vertices[i0].m_pos;
			glm::vec3 v2 = vertices[i2].m_pos - vertices[i0].m_pos;

			glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

			vertices[i0].m_normal = vertices[i0].m_normal + normal;
			vertices[i1].m_normal = vertices[i0].m_normal + normal;
			vertices[i2].m_normal = vertices[i0].m_normal + normal;

		}

		std::for_each(vertices.begin(), vertices.end(), [](GLVertexData val)
		{
			val.m_normal = glm::normalize(val.m_normal);
		});
	}

	std::vector<float> verticesBuffer;

	std::for_each(vertices.begin(), vertices.end(), [&](GLVertexData val)
	{
		verticesBuffer.emplace_back(val.m_pos.x);
		verticesBuffer.emplace_back(val.m_pos.y);
		verticesBuffer.emplace_back(val.m_pos.z);
		verticesBuffer.emplace_back(val.m_texCoord.x);
		verticesBuffer.emplace_back(val.m_texCoord.y);
		verticesBuffer.emplace_back(val.m_normal.x);
		verticesBuffer.emplace_back(val.m_normal.y);
		verticesBuffer.emplace_back(val.m_normal.z);
	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);

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
			LogManager::getInstance().printLog("Error: Failed to load cubemap texture: " + ("../res/textures/" + faceImagePath[i]));
		}
		stbi_image_free(data);
	}
}
