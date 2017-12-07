#include "../../../main/stdafx.h"
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

void GLMeshData::draw(std::vector<unsigned int>& indices, meshDrawMethod meshDrawMethod)
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES + (int)meshDrawMethod, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
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
		verticesBuffer.emplace_back(val.m_tangent.x);
		verticesBuffer.emplace_back(val.m_tangent.y);
		verticesBuffer.emplace_back(val.m_tangent.z);
		verticesBuffer.emplace_back(val.m_bitangent.x);
		verticesBuffer.emplace_back(val.m_bitangent.y);
		verticesBuffer.emplace_back(val.m_bitangent.z);

	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesBuffer.size() * sizeof(float), &verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(5 * sizeof(float)));

	// tangent coord attribute, 4th attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));

	// bitangent coord attribute, 5th attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

GLTextureData::GLTextureData()
{
}

GLTextureData::~GLTextureData()
{
}

void GLTextureData::init(textureType textureType, textureWrapMethod textureWrapMethod)
{
	GLint l_textureWrapMethod;
	switch (textureWrapMethod)
	{
	case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
	case textureWrapMethod::CLAMPTOEDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	}
	switch (textureType)
	{
	case textureType::INVISIBLE: break;
	case textureType::DIFFUSE:
		// @TODO: more general texture parameter
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case textureType::SPECULAR:
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case textureType::NORMALS:
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case textureType::CUBEMAP:
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
}

void GLTextureData::draw(textureType textureType)
{
	switch (textureType)
	{
	case textureType::INVISIBLE: break;
	case textureType::DIFFUSE:
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::SPECULAR:
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::NORMALS:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::CUBEMAP:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		break;
	}
}

void GLTextureData::shutdown()
{
}

void GLTextureData::sendDataToGPU(textureType textureType, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void* textureData) const
{
	GLenum l_internalFormat;
	if (textureFormat == 1)
	{
		l_internalFormat = GL_RED;
	}
	else if (textureFormat == 3)
	{
		if (textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB;
		}
		else
		{
			l_internalFormat = GL_RGB;
		}
	}
	else if (textureFormat == 4)
	{
		if (textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
		else
		{
			l_internalFormat = GL_RGBA;
		}
	}

	switch (textureType)
	{
	case textureType::INVISIBLE: break;
	case textureType::DIFFUSE:
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, textureWidth, textureHeight, 0, l_internalFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		break;
	case textureType::SPECULAR:
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, textureWidth, textureHeight, 0, l_internalFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		break;
	case textureType::NORMALS:
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, textureWidth, textureHeight, 0, l_internalFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		break;
	case textureType::CUBEMAP:
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, 0, l_internalFormat, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
		break;
	}
}

