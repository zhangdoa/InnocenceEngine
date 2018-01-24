#include "../../main/stdafx.h"
#include "GraphicData.h"

Vertex::Vertex()
{
}

Vertex::Vertex(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
}

Vertex& Vertex::operator=(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
	return *this;
}

Vertex::Vertex(const vec3& pos, const vec2& texCoord, const vec3& normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

Vertex::~Vertex()
{
}

void IMesh::setup()
{
	this->setup(meshDrawMethod::TRIANGLE, false, false);
}

void IMesh::setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents)
{
	m_meshDrawMethod = meshDrawMethod;
	m_calculateNormals = calculateNormals;
	m_calculateTangents = calculateTangents;

	if (m_calculateNormals)
	{
		for (auto& l_vertices : m_vertices)
		{
			l_vertices.m_normal = l_vertices.m_pos;
		}
	}
}

void IMesh::addVertices(const Vertex & Vertex)
{
	m_vertices.emplace_back(Vertex);
}

void IMesh::addVertices(const vec3 & pos, const vec2 & texCoord, const vec3 & normal)
{

}

void IMesh::addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z)
{
	m_vertices.emplace_back(Vertex(vec3(pos_x, pos_y, pos_z), vec2(texCoord_x, texCoord_y), vec3(normal_x, normal_y, normal_z)));
}

void IMesh::addIndices(unsigned int index)
{
	m_indices.emplace_back(index);
}

void IMesh::addUnitCube()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec3(1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec3(1.0f, -1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec3(-1.0f, -1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec3(-1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec3(1.0f, 1.0f, -1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec3(1.0f, -1.0f, -1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec3(-1.0f, -1.0f, -1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec3(-1.0f, 1.0f, -1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);


	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : m_vertices)
	{
		l_vertexData.m_normal = l_vertexData.m_pos.normalize();
		//l_vertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_vertexData.m_normal));
		//l_vertexData.m_bitangent = glm::normalize(glm::cross(l_vertexData.m_tangent, l_vertexData.m_normal));
	}
	m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void IMesh::addUnitSphere()
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	double PI = 3.14159265359;

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			double xPos = glm::cos(xSegment * 2.0f * PI) * glm::sin(ySegment * PI);
			double yPos = glm::cos(ySegment * PI);
			double zPos = glm::sin(xSegment * 2.0f * PI) * glm::sin(ySegment * PI);

			Vertex l_VertexData;
			l_VertexData.m_pos = vec3(xPos, yPos, zPos);
			l_VertexData.m_texCoord = vec2(xSegment, ySegment);
			l_VertexData.m_normal = vec3(xPos, yPos, zPos).normalize();
			//l_VertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_VertexData.m_normal));
			//l_VertexData.m_bitangent = glm::normalize(glm::cross(l_VertexData.m_tangent, l_VertexData.m_normal));
			m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
}

void IMesh::addUnitQuad()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec3(1.0f, 1.0f, 0.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec3(1.0f, -1.0f, 0.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec3(-1.0f, -1.0f, 0.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec3(-1.0f, 1.0f, 0.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	m_indices = { 0, 1, 3, 1, 2, 3 };
}

meshID IMesh::getMeshDataID() const
{
	return this->getEntityID();
}

void GLMesh::initialize()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
	{
		l_verticesBuffer.emplace_back(val.m_pos.x);
		l_verticesBuffer.emplace_back(val.m_pos.y);
		l_verticesBuffer.emplace_back(val.m_pos.z);
		l_verticesBuffer.emplace_back(val.m_texCoord.x);
		l_verticesBuffer.emplace_back(val.m_texCoord.y);
		l_verticesBuffer.emplace_back(val.m_normal.x);
		l_verticesBuffer.emplace_back(val.m_normal.y);
		l_verticesBuffer.emplace_back(val.m_normal.z);
	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(float), &m_indices[0], GL_STATIC_DRAW);

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

	setStatus(objectStatus::ALIVE);
}

void GLMesh::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glActiveTexture(GL_TEXTURE0);
	}
}

void GLMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	setStatus(objectStatus::SHUTDOWN);
}

void ITexture::setup()
{
	this->setup(textureType::DIFFUSE, textureWrapMethod::REPEAT, 0, 0, 0, 0, nullptr);
}

void ITexture::setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData)
{
	m_textureType = textureType;
	m_textureWrapMethod = textureWrapMethod;
	m_textureIndex = textureIndex;
	m_textureFormat = textureFormat;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_textureRawData = textureData;
}

textureID ITexture::getTextureDataID() const
{
	return this->getEntityID();
}

void GLTexture::initialize()
{
	GLint l_textureWrapMethod;
	switch (m_textureWrapMethod)
	{
	case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
	case textureWrapMethod::CLAMPTOEDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	}
	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else if (m_textureType == textureType::CUBEMAP)
	{
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		// @TODO: more general texture parameter
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	GLenum l_internalFormat;
	if (m_textureFormat == 1)
	{
		l_internalFormat = GL_RED;
	}
	else if (m_textureFormat == 3)
	{
		if (m_textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB;
		}
		else
		{
			l_internalFormat = GL_RGB;
		}
	}
	else if (m_textureFormat == 4)
	{
		if (m_textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
		else
		{
			l_internalFormat = GL_RGBA;
		}
	}

	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else if (m_textureType == textureType::CUBEMAP)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + m_textureIndex, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_internalFormat, GL_UNSIGNED_BYTE, m_textureRawData);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	setStatus(objectStatus::ALIVE);
}

void GLTexture::update()
{
	//@TODO: switch is too slow for CPU
	switch (m_textureType)
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
	case textureType::AMBIENT:
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::EMISSIVE:
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::CUBEMAP:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		break;
	}
}

void GLTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}

