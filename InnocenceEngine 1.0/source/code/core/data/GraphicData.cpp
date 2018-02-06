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
			double xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
			double yPos = cos(ySegment * PI);
			double zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

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

void I2DTexture::setup()
{
	this->setup(textureType::ALBEDO, textureWrapMethod::REPEAT, 0, 0, 0, nullptr);
}

void I2DTexture::setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureFormat, int textureWidth, int textureHeight, void * textureData)
{
	m_textureType = textureType;
	m_textureWrapMethod = textureWrapMethod;
	m_textureFormat = textureFormat;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_textureRawData = textureData;
}

void GL2DTexture::initialize()
{
	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else
	{
		GLint l_textureWrapMethod;
		switch (m_textureWrapMethod)
		{
		case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
		case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
		}

		// @TODO: more general texture parameter
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		GLenum l_internalFormat;
		GLenum l_dataFormat;
		if (m_textureType == textureType::ALBEDO)
		{
			if (m_textureFormat == 1)
			{
				l_internalFormat = GL_RED;
				l_dataFormat = GL_RED;
			}
			else if (m_textureFormat == 3)
			{

				l_internalFormat = GL_SRGB;
				l_dataFormat = GL_RGB;
			}
			else if (m_textureFormat == 4)
			{
				l_internalFormat = GL_SRGB_ALPHA;
				l_dataFormat = GL_RGBA;
			}
		}
		else 
		{
			if (m_textureFormat == 1)
			{
				l_internalFormat = GL_RED;
				l_dataFormat = GL_RED;
			}
			else if (m_textureFormat == 3)
			{
				l_internalFormat = GL_RGB;
				l_dataFormat = GL_RGB;
			}
			else if (m_textureFormat == 4)
			{
				l_internalFormat = GL_RGBA;
				l_dataFormat = GL_RGBA;
			}
		}
		
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, GL_UNSIGNED_BYTE, m_textureRawData);
		glGenerateMipmap(GL_TEXTURE_2D);

		setStatus(objectStatus::ALIVE);
	}	
}

void GL2DTexture::update()
{
	this->update(0);
}

void GL2DTexture::update(int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureIndex);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void GL2DTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}

void GL2DHDRTexture::initialize()
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData);

	setStatus(objectStatus::ALIVE);
}

void GL2DHDRTexture::update()
{
	this->update(0);
}

void GL2DHDRTexture::update(int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureIndex);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void GL2DHDRTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}

void I3DTexture::setup()
{
}

void I3DTexture::setup(textureType textureType, int textureFormat, int textureWidth, int textureHeight, const std::vector<void *>& textureData, bool generateMipMap)
{
	m_textureType = textureType;
	m_textureFormat = textureFormat;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_generateMipMap = generateMipMap;

	m_textureRawData_Right = textureData[0];
	m_textureRawData_Left = textureData[1];
	m_textureRawData_Top = textureData[2];
	m_textureRawData_Bottom = textureData[3];
	m_textureRawData_Back = textureData[4];
	m_textureRawData_Front = textureData[5];
}

void GL3DTexture::initialize()
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters

	if (m_generateMipMap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);	
	}
	else
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum l_internalFormat;
	if (m_textureFormat == 1)
	{
		l_internalFormat = GL_RED;
	}
	else if (m_textureFormat == 3)
	{

		l_internalFormat = GL_SRGB;
	}
	else if (m_textureFormat == 4)
	{
		l_internalFormat = GL_SRGB_ALPHA;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Right);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Left);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Top);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Bottom);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Back);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData_Front);

	if (m_generateMipMap)
	{
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	setStatus(objectStatus::ALIVE);
}

void GL3DTexture::update()
{
	this->update(0);
}

void GL3DTexture::update(int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureIndex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
}

void GL3DTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}

void GL3DHDRTexture::initialize()
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters

	if (m_generateMipMap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Right);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Left);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Top);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Bottom);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Back);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureRawData_Front);

	if (m_generateMipMap)
	{
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	setStatus(objectStatus::ALIVE);
}

void GL3DHDRTexture::update()
{
	this->update(0);
}

void GL3DHDRTexture::update(int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureIndex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
}

void GL3DHDRTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}

void GL3DHDRTexture::updateFramebuffer(int index, int mipLevel)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, m_textureID, mipLevel);
}

void IFrameBuffer::setup()
{
}

void IFrameBuffer::setup(vec2 renderBufferStorageResolution, bool isDeferPass, unsigned int renderTargetTextureNumber)
{
	m_renderBufferStorageResolution = renderBufferStorageResolution;
	m_isDeferPass = isDeferPass;
	m_renderTargetTextureNumber = renderTargetTextureNumber;
}

void GLFrameBuffer::initialize()
{
	//generate and bind frame buffer
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
	//@TODO: not only this static 24 + 8
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_renderBufferStorageResolution.x, (int)m_renderBufferStorageResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

	std::vector<unsigned int> attachments;
	for (auto i = 0; i < m_renderTargetTextureNumber; i++)
	{
		m_textures.emplace_back();
		attachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	for (auto i = 0; i < m_textures.size(); i++)
	{
		glGenTextures(1, &m_textures[i]);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_renderBufferStorageResolution.x, (int)m_renderBufferStorageResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
	}
	glDrawBuffers(attachments.size(), &attachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Framebuffer is not completed!");
	}

	if (m_isDeferPass)
	{	
		m_Vertices = {
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		// take care of std::vector's size and pointer of first element!!!
		glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(float), &m_Vertices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::update()
{
	glBindRenderbuffer(GL_RENDERBUFFER, m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_RBO);
}

void GLFrameBuffer::activeTexture(int textureLevel, int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureLevel);
	glBindTexture(GL_TEXTURE_2D, m_textures[textureIndex]);
}

void GLFrameBuffer::drawMesh()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLFrameBuffer::shutdown()
{
	glDeleteFramebuffers(1, &m_FBO);
	glDeleteRenderbuffers(1, &m_RBO);
}
