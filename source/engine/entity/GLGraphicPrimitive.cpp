#include "GLGraphicPrimitive.h"

void GLMesh::initialize()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;

	if (m_meshType == meshType::TWO_DIMENSION)
	{
		std::for_each(m_vertices.begin(), m_vertices.end(), [&](Vertex val)
		{
			l_verticesBuffer.emplace_back((float)val.m_pos.x);
			l_verticesBuffer.emplace_back((float)val.m_pos.y);
			l_verticesBuffer.emplace_back((float)val.m_pos.z);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
		});
	}
	else
	{
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
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW);

	if (m_meshType == meshType::TWO_DIMENSION)
	{
		// position attribute, 1st attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

		// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	else
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
	}

	m_objectStatus = objectStatus::ALIVE;
}

void GLMesh::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void GLMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GLTexture::initialize()
{
	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else
	{
		//generate and bind texture object
		glGenTextures(1, &m_textureID);
		if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_textureID);
		}
		
		// set the texture wrapping parameters
		GLenum l_textureWrapMethod;
		switch (m_textureWrapMethod)
		{
		case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
		case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
		}
		if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		}

		// set texture filtering parameters
		GLenum l_minFilterParam;
		switch (m_textureMinFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_minFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_minFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		GLenum l_magFilterParam;
		switch (m_textureMagFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_magFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_magFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
		}

		// set texture formats
		GLenum l_internalFormat;
		GLenum l_dataFormat;
		GLenum l_type;
		if (m_textureType == textureType::ALBEDO)
		{
			if (m_texturePixelDataFormat == texturePixelDataFormat::RGB)
			{

				l_internalFormat = GL_SRGB;
			}
			else if (m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
			{
				l_internalFormat = GL_SRGB_ALPHA;
			}
		}
		else 
		{
			switch (m_textureColorComponentsFormat)
			{	
				case textureColorComponentsFormat::RED: l_internalFormat = GL_RED; break;
				case textureColorComponentsFormat::RG: l_internalFormat = GL_RG; break;
				case textureColorComponentsFormat::RGB: l_internalFormat = GL_RGB; break;
				case textureColorComponentsFormat::RGBA: l_internalFormat = GL_RGBA; break;
				case textureColorComponentsFormat::R8: l_internalFormat = GL_R8; break;
				case textureColorComponentsFormat::RG8: l_internalFormat = GL_RG8; break;
				case textureColorComponentsFormat::RGB8: l_internalFormat = GL_RGB8; break;
				case textureColorComponentsFormat::RGBA8: l_internalFormat = GL_RGBA8; break;
				case textureColorComponentsFormat::R16: l_internalFormat = GL_R16; break;
				case textureColorComponentsFormat::RG16: l_internalFormat = GL_RG16; break;
				case textureColorComponentsFormat::RGB16: l_internalFormat = GL_RGB16; break;
				case textureColorComponentsFormat::RGBA16: l_internalFormat = GL_RGBA16; break;
				case textureColorComponentsFormat::R16F: l_internalFormat = GL_R16F; break;
				case textureColorComponentsFormat::RG16F: l_internalFormat = GL_RG16F; break;
				case textureColorComponentsFormat::RGB16F: l_internalFormat = GL_RGB16F; break;
				case textureColorComponentsFormat::RGBA16F: l_internalFormat = GL_RGBA16F; break;
				case textureColorComponentsFormat::R32F: l_internalFormat = GL_R32F; break;
				case textureColorComponentsFormat::RG32F: l_internalFormat = GL_RG32F; break;
				case textureColorComponentsFormat::RGB32F: l_internalFormat = GL_RGB32F; break;
				case textureColorComponentsFormat::RGBA32F: l_internalFormat = GL_RGBA32F; break;
				case textureColorComponentsFormat::SRGB: l_internalFormat = GL_SRGB; break;
				case textureColorComponentsFormat::SRGBA: l_internalFormat = GL_SRGB_ALPHA; break;
				case textureColorComponentsFormat::SRGB8: l_internalFormat = GL_SRGB8; break;
				case textureColorComponentsFormat::SRGBA8: l_internalFormat = GL_SRGB8_ALPHA8; break;
			}
		}
		switch (m_texturePixelDataFormat)
		{
			case texturePixelDataFormat::RED:l_dataFormat = GL_RED; break;
			case texturePixelDataFormat::RG:l_dataFormat = GL_RG; break;
			case texturePixelDataFormat::RGB:l_dataFormat = GL_RGB; break;
			case texturePixelDataFormat::RGBA:l_dataFormat = GL_RGBA; break;
		}
		switch (m_texturePixelDataType)
		{
		case texturePixelDataType::UNSIGNED_BYTE:l_type = GL_UNSIGNED_BYTE; break;
		case texturePixelDataType::BYTE:l_type = GL_BYTE; break;
		case texturePixelDataType::UNSIGNED_SHORT:l_type = GL_UNSIGNED_SHORT; break;
		case texturePixelDataType::SHORT:l_type = GL_SHORT; break;
		case texturePixelDataType::UNSIGNED_INT:l_type = GL_UNSIGNED_INT; break;
		case texturePixelDataType::INT:l_type = GL_INT; break;
		case texturePixelDataType::FLOAT:l_type = GL_FLOAT; break;
		}
		
		if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[0]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[1]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[2]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[3]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[4]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[5]);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_dataFormat, l_type, m_textureData[0]);
		}
		
		// should generate mipmap or not
		if (m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
			else
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}
		}
		
		m_objectStatus = objectStatus::ALIVE;
	}	
}

void GLTexture::update()
{
	this->update(0);
}

void GLTexture::update(int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureIndex);
	if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_textureID);
	}
}

void GLTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	m_objectStatus = objectStatus::SHUTDOWN;
}

void GLTexture::updateFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel)
{
	if (m_textureType == textureType::CUBEMAP || m_textureType == textureType::ENVIRONMENT_CAPTURE || m_textureType == textureType::ENVIRONMENT_CONVOLUTION || m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, m_textureID, mipLevel);
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, m_textureID, mipLevel);
	}
}

GLShader::GLShader()
{
}

GLShader::~GLShader()
{
}

void GLShader::initialize()
{
	int l_glShaderType = 0;

	switch (m_shaderType)
	{
	case shaderType::VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case shaderType::GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case shaderType::FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: g_pLogSystem->printLog("Error: Unknown shader type, cannot create shader!"); m_objectStatus = objectStatus::STANDBY; break;
	}

	m_shaderID = glCreateShader(l_glShaderType);

	if (m_shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
		m_objectStatus = objectStatus::STANDBY;
	}

	char const * l_sourcePointer = m_shaderCode.c_str();
	glShaderSource(m_shaderID, 1, &l_sourcePointer, NULL);
}

void GLShader::update()
{
}

void GLShader::shutdown()
{
	glDeleteShader(m_shaderID);
	m_objectStatus = objectStatus::SHUTDOWN;
}

const GLint & GLShader::getShaderID() const
{
	return m_shaderID;
}

GLShaderProgram::GLShaderProgram()
{
}

GLShaderProgram::~GLShaderProgram()
{
}

void GLShaderProgram::initialize()
{
	m_program = glCreateProgram();

	if (m_vertexShader.getStatus() == objectStatus::ALIVE)
	{
		m_vertexShader.initialize();
		attachShader(&m_vertexShader);
		for (unsigned int i = 0; i < m_vertexShader.getAttributions().size(); i++)
		{
			setAttributeLocation(i, m_vertexShader.getAttributions()[i]);
		}
	}
	if (m_geometryShader.getStatus() == objectStatus::ALIVE)
	{
		m_geometryShader.initialize();
		attachShader(&m_geometryShader);
	}
	if (m_fragmentShader.getStatus() == objectStatus::ALIVE)
	{
		m_fragmentShader.initialize();
		attachShader(&m_fragmentShader);
	}
}

void GLShaderProgram::shutdown()
{
	if (m_vertexShader.getStatus() == objectStatus::ALIVE)
	{
		m_vertexShader.shutdown();
		glDetachShader(m_program, m_vertexShader.getShaderID());
	}
	if (m_geometryShader.getStatus() == objectStatus::ALIVE)
	{
		m_geometryShader.shutdown();
		glDetachShader(m_program, m_geometryShader.getShaderID());
	}
	if (m_fragmentShader.getStatus() == objectStatus::ALIVE)
	{
		m_fragmentShader.shutdown();
		glDetachShader(m_program, m_fragmentShader.getShaderID());
	}
}

void GLShaderProgram::attachShader(const GLShader* GLShader) const
{
	GLint Result = GL_FALSE;
	int l_infoLogLength = 0;

	glGetShaderiv(m_program, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(m_program, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (l_infoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(m_program, l_infoLogLength, NULL, &ShaderErrorMessage[0]);
		g_pLogSystem->printLog(&ShaderErrorMessage[0]);
	}

	const GLint l_shaderID = GLShader->getShaderID();

	glAttachShader(m_program, l_shaderID);
	glLinkProgram(m_program);
	glValidateProgram(m_program);

	g_pLogSystem->printLog("innoShader: " + GLShader->getShaderFilePath() + " Shader is compiled.");

	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shaderID, 1024, NULL, infoLog);
		g_pLogSystem->printLog("innoShader: Error: Shader compile error: " + std::string(infoLog) + "\n -- --------------------------------------------------- -- ");
	}
}

void GLShaderProgram::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Attribute lost: " + arrtributeName);
	}
}

inline void GLShaderProgram::useProgram() const
{
	glUseProgram(m_program);
}

inline void GLShaderProgram::addUniform(std::string uniform) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Uniform lost: " + uniform);
	}
}

inline GLint GLShaderProgram::getUniformLocation(const std::string & uniformName) const
{
	return glGetUniformLocation(m_program, uniformName.c_str());
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, bool uniformValue) const
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, int uniformValue) const
{
	glUniform1i(uniformLocation, uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double uniformValue) const
{
	glUniform1f(uniformLocation, uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, x, y);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, x, y, z);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z, double w)
{
	glUniform4f(uniformLocation, x, y, z, w);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, const mat4 & mat) const
{
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
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
	switch (m_renderBufferType)
	{
	case renderBufferType::DEPTH:l_internalformat = GL_DEPTH_COMPONENT24; l_attachment = GL_DEPTH_ATTACHMENT; break;
	case renderBufferType::STENCIL:l_internalformat = GL_STENCIL_INDEX16; l_attachment = GL_STENCIL_ATTACHMENT; break;
	case renderBufferType::DEPTH_AND_STENCIL: l_internalformat = GL_DEPTH24_STENCIL8; l_attachment = GL_DEPTH_STENCIL_ATTACHMENT; break;
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

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::update()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
}

void GLFrameBuffer::activeTexture(int textureLevel, int textureIndex)
{
	glActiveTexture(GL_TEXTURE0 + textureLevel);
	glBindTexture(GL_TEXTURE_2D, m_textures[textureIndex]);
}

void GLFrameBuffer::shutdown()
{
	glDeleteFramebuffers(1, &m_FBO);
	glDeleteRenderbuffers(1, &m_RBO);
}

void GLFrameBufferWIP::initialize()
{
	//generate and bind frame buffer
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// generate and bind render buffer
	glGenRenderbuffers(1, &m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);

	switch (m_renderBufferType)
	{
	case renderBufferType::DEPTH:m_internalformat = GL_DEPTH_COMPONENT32; m_attachment = GL_DEPTH_ATTACHMENT; break;
	case renderBufferType::STENCIL:m_internalformat = GL_STENCIL_INDEX16; m_attachment = GL_STENCIL_ATTACHMENT; break;
	case renderBufferType::DEPTH_AND_STENCIL: m_internalformat = GL_DEPTH24_STENCIL8; m_attachment = GL_DEPTH_STENCIL_ATTACHMENT; break;
	}

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_attachment, GL_RENDERBUFFER, m_RBO);

	for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); i++)
	{
		m_renderTargetTextures[i]->initialize();
		m_renderTargetTextures[i]->updateFramebuffer(i, 0, 0);
	}

	for (auto i = (unsigned int)0; i < m_shaderPrograms.size(); i++)
	{
		m_shaderPrograms[i]->initialize();
	}

	std::vector<unsigned int> attachments;
	for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); i++)
	{
		attachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	
	glDrawBuffers(attachments.size(), &attachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBufferWIP::update()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);

	if (m_previousBaseFrameBuffer)
	{
		for (auto i = (unsigned int)0; i < m_previousBaseFrameBuffer->getRenderTargetNumber(); i++)
		{
			m_previousBaseFrameBuffer->activeTexture(i, i, 0);
		}
	}
	for (auto i = (unsigned int)0; i < m_shaderPrograms.size(); i++)
	{
		glRenderbufferStorage(GL_RENDERBUFFER, m_internalformat, m_renderBufferStorageSize[i].x, m_renderBufferStorageSize[i].y);
		glViewport(0, 0, m_renderBufferStorageSize[i].x, m_renderBufferStorageSize[i].y);
		m_shaderPrograms[i]->update();
	}
}

void GLFrameBufferWIP::activeTexture(int colorAttachmentIndex, int textureIndex, int textureMipMapLevel)
{
	glRenderbufferStorage(GL_RENDERBUFFER, m_internalformat, m_renderTargetTextures[textureIndex]->getTextureWidth(), m_renderTargetTextures[textureIndex]->getTextureHeight());
	glViewport(0, 0, m_renderTargetTextures[textureIndex]->getTextureWidth(), m_renderTargetTextures[textureIndex]->getTextureHeight());
	m_renderTargetTextures[textureIndex]->updateFramebuffer(colorAttachmentIndex, textureIndex, textureMipMapLevel);
	m_renderTargetTextures[textureIndex]->update(textureIndex);
}

void GLFrameBufferWIP::shutdown()
{
	for (auto i = (unsigned int)0; i < m_renderTargetTextures.size(); i++)
	{
		m_renderTargetTextures[i]->shutdown();
	}
	glDeleteFramebuffers(1, &m_FBO);
	glDeleteRenderbuffers(1, &m_RBO);
}
