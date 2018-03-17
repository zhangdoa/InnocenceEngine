#include "GLGraphicPrimitive.h"

void GL2DMesh::initialize()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
	{
		l_verticesBuffer.emplace_back((float)val.m_pos.x);
		l_verticesBuffer.emplace_back((float)val.m_pos.y);
		l_verticesBuffer.emplace_back((float)val.m_pos.z);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	m_objectStatus = objectStatus::ALIVE;
}

void GL2DMesh::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void GL2DMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL3DMesh::initialize()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
	{
		l_verticesBuffer.emplace_back((float)val.m_pos.x);
		l_verticesBuffer.emplace_back((float)val.m_pos.y);
		l_verticesBuffer.emplace_back((float)val.m_pos.z);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
		l_verticesBuffer.emplace_back((float)val.m_normal.x);
		l_verticesBuffer.emplace_back((float)val.m_normal.y);
		l_verticesBuffer.emplace_back((float)val.m_normal.z);
	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW);

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

	m_objectStatus = objectStatus::ALIVE;
}

void GL3DMesh::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void GL3DMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL2DTexture::initialize()
{
	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else
	{
		//generate and bind texture object
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		
		// set the texture wrapping parameters
		GLenum l_textureWrapMethod;
		switch (m_textureWrapMethod)
		{
		case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
		case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);

		// set texture filtering parameters
		GLenum l_minFilterParam;
		if (m_textureMinFilterMethod == textureFilterMethod::NEAREST)
		{
			l_minFilterParam = GL_NEAREST;
		}
		else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR)
		{
			l_minFilterParam = GL_LINEAR;
		}
		else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR;
		}
		GLenum l_magFilterParam;
		if (m_textureMagFilterMethod == textureFilterMethod::NEAREST)
		{
			l_magFilterParam = GL_NEAREST;
		}
		else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR)
		{
			l_magFilterParam = GL_LINEAR;
		}
		else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
		
		// set texture formats
		GLenum l_internalFormat;
		GLenum l_dataFormat;
		if (m_textureType == textureType::ALBEDO)
		{
			if (m_texturePixelDataFormat == texturePixelDataFormat::RED)
			{
				l_internalFormat = GL_RED;
				l_dataFormat = GL_RED;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RG)
			{

				l_internalFormat = GL_RG;
				l_dataFormat = GL_RG;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RGB)
			{

				l_internalFormat = GL_SRGB;
				l_dataFormat = GL_RGB;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
			{
				l_internalFormat = GL_SRGB_ALPHA;
				l_dataFormat = GL_RGBA;
			}
		}
		else 
		{
			if (m_texturePixelDataFormat == texturePixelDataFormat::RED)
			{
				l_internalFormat = GL_RED;
				l_dataFormat = GL_RED;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RG)
			{
				l_internalFormat = GL_RG;
				l_dataFormat = GL_RG;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RGB)
			{
				l_internalFormat = GL_RGB;
				l_dataFormat = GL_RGB;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
			{
				l_internalFormat = GL_RGBA;
				l_dataFormat = GL_RGBA;
			}
		}
		
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, GL_UNSIGNED_BYTE, m_textureData[0]);
		
		// should generate mipmap or not
		if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
		m_objectStatus = objectStatus::ALIVE;
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

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL2DHDRTexture::initialize()
{
	//generate and bind texture object
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// set texture filtering parameters
	GLenum l_minFilterParam;
	if (m_textureMinFilterMethod == textureFilterMethod::NEAREST)
	{
		l_minFilterParam = GL_NEAREST;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR)
	{
		l_minFilterParam = GL_LINEAR;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}
	GLenum l_magFilterParam;
	if (m_textureMagFilterMethod == textureFilterMethod::NEAREST)
	{
		l_magFilterParam = GL_NEAREST;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR)
	{
		l_magFilterParam = GL_LINEAR;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[0]);

	m_objectStatus = objectStatus::ALIVE;
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

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL3DTexture::initialize()
{
	//generate and bind texture object
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	// set texture filtering parameters
	GLenum l_minFilterParam;
	if (m_textureMinFilterMethod == textureFilterMethod::NEAREST)
	{
		l_minFilterParam = GL_NEAREST;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR)
	{
		l_minFilterParam = GL_LINEAR;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}
	GLenum l_magFilterParam;
	if (m_textureMagFilterMethod == textureFilterMethod::NEAREST)
	{
		l_magFilterParam = GL_NEAREST;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR)
	{
		l_magFilterParam = GL_LINEAR;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, l_magFilterParam);

	// set texture formats
	GLenum l_internalFormat;
	if (m_textureColorComponentsFormat == textureColorComponentsFormat::RED)
	{
		l_internalFormat = GL_RED;
	}
	else if (m_textureColorComponentsFormat == textureColorComponentsFormat::RG)
	{

		l_internalFormat = GL_RG;
	}
	else if (m_textureColorComponentsFormat == textureColorComponentsFormat::RGB)
	{

		l_internalFormat = GL_SRGB;
	}
	else if (m_textureColorComponentsFormat == textureColorComponentsFormat::RGBA)
	{
		l_internalFormat = GL_SRGB_ALPHA;
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[1]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[2]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[3]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[4]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureData[5]);

	// should generate mipmap or not
	if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	m_objectStatus = objectStatus::ALIVE;
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

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL3DHDRTexture::initialize()
{
	//generate and bind texture object
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	// set texture filtering parameters
	GLenum l_minFilterParam;
	if (m_textureMinFilterMethod == textureFilterMethod::NEAREST)
	{
		l_minFilterParam = GL_NEAREST;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR)
	{
		l_minFilterParam = GL_LINEAR;
	}
	else if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}
	GLenum l_magFilterParam;
	if (m_textureMagFilterMethod == textureFilterMethod::NEAREST)
	{
		l_magFilterParam = GL_NEAREST;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR)
	{
		l_magFilterParam = GL_LINEAR;
	}
	else if (m_textureMagFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, l_magFilterParam);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[1]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[2]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[3]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[4]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_FLOAT, m_textureData[5]);

	// should generate mipmap or not
	if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	m_objectStatus = objectStatus::ALIVE;
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

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GL3DHDRTexture::updateFramebuffer(int index, int mipLevel)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, m_textureID, mipLevel);
}

void GLFrameBuffer::initialize()
{
	//generate and bind frame buffer
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// create and attach render buffer
	glGenRenderbuffers(1, &m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);

	GLenum l_internalformat;
	GLenum l_attachment;
	if (m_renderBufferType == renderBufferType::DEPTH)
	{
		l_internalformat = GL_DEPTH_COMPONENT24;
		l_attachment = GL_DEPTH_ATTACHMENT;
	}
	else if (m_renderBufferType == renderBufferType::STENCIL)
	{
		l_internalformat = GL_STENCIL_INDEX16;
		l_attachment = GL_STENCIL_ATTACHMENT;
	}
	else if (m_renderBufferType == renderBufferType::DEPTH_AND_STENCIL)
	{
		l_internalformat = GL_DEPTH24_STENCIL8;
		l_attachment = GL_DEPTH_STENCIL_ATTACHMENT;
	}
	glRenderbufferStorage(GL_RENDERBUFFER, l_internalformat, (int)m_renderBufferStorageResolution.x, (int)m_renderBufferStorageResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, l_attachment, GL_RENDERBUFFER, m_RBO);

	std::vector<unsigned int> attachments;
	for (auto i = (unsigned int)0; i < m_renderTargetTextureNumber; i++)
	{
		m_textures.emplace_back();
		attachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	for (auto i = (unsigned int)0; i < m_textures.size(); i++)
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
		g_pLogSystem->printLog("Framebuffer is not completed!");
	}

	if (m_frameBufferType == frameBufferType::DEFER)
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

		//m_frameBufferMesh = this->getMesh(meshType::TWO_DIMENSION, this->addMesh(meshType::TWO_DIMENSION));
		//m_frameBufferMesh->addUnitQuad();
		//m_frameBufferMesh->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
		//m_frameBufferMesh->initialize();
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
