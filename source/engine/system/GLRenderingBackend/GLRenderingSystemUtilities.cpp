#include "GLRenderingSystemUtilities.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void getGLError()
	{
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(err));
		}
	}

	void generateFBO(GLRenderPassComponent* GLRPC);
	void generateRBO(GLRenderPassComponent* GLRPC);

	void addRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc);
	void attachRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex);

	void setDrawBuffers(unsigned int RTNum);

	void addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath);

	bool summitGPUData(GLMeshDataComponent* rhs);
	bool summitGPUData(GLTextureDataComponent* rhs);

	GLTextureDataDesc getGLTextureDataDesc(TextureDataDesc textureDataDesc);

	GLenum getTextureSamplerType(TextureSamplerType rhs);
	GLenum getTextureWrapMethod(TextureWrapMethod rhs);
	GLenum getTextureFilterParam(TextureFilterMethod rhs);
	GLenum getTextureInternalFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataType(TexturePixelDataType rhs);

	void generateTO(GLuint& TO, GLTextureDataDesc desc, const std::vector<void*>& textureData);

	std::unordered_map<InnoEntity*, GLMeshDataComponent*> m_initializedGLMDC;
	std::unordered_map<InnoEntity*, GLTextureDataComponent*> m_initializedGLTDC;

	void* m_GLRenderPassComponentPool;
	void* m_GLShaderProgramComponentPool;

	const std::string m_shaderRelativePath = std::string{ "res//shaders//" };
}

bool GLRenderingSystemNS::initializeComponentPool()
{
	m_GLRenderPassComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(GLRenderPassComponent), 128);
	m_GLShaderProgramComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(GLShaderProgramComponent), 256);

	return true;
}

GLRenderPassComponent* GLRenderingSystemNS::addGLRenderPassComponent(const EntityID& parentEntity, const char* name)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_GLRenderPassComponentPool, sizeof(GLRenderPassComponent));
	auto l_GLRPC = new(l_rawPtr)GLRenderPassComponent();
	l_GLRPC->m_parentEntity = parentEntity;
	l_GLRPC->m_name = name;

	return l_GLRPC;
}

bool GLRenderingSystemNS::initializeGLRenderPassComponent(GLRenderPassComponent* GLRPC)
{
	generateFBO(GLRPC);
	if (GLRPC->m_renderPassDesc.useDepthAttachment)
	{
		GLRPC->m_renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
		GLRPC->m_renderBufferInternalFormat = GL_DEPTH_COMPONENT32F;

		if (GLRPC->m_renderPassDesc.useStencilAttachment)
		{
			GLRPC->m_renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
			GLRPC->m_renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
		}
		generateRBO(GLRPC);
	}

	GLRPC->m_GLTDCs.reserve(GLRPC->m_renderPassDesc.RTNumber);

	for (unsigned int i = 0; i < GLRPC->m_renderPassDesc.RTNumber; i++)
	{
		GLRPC->m_GLTDCs.emplace_back();
	}

	for (unsigned int i = 0; i < GLRPC->m_renderPassDesc.RTNumber; i++)
	{
		auto l_TDC = addGLTextureDataComponent();

		l_TDC->m_textureDataDesc = GLRPC->m_renderPassDesc.RTDesc;

		if (l_TDC->m_textureDataDesc.samplerType == TextureSamplerType::CUBEMAP)
		{
			l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		}
		else
		{
			l_TDC->m_textureData = { nullptr };
		}

		initializeGLTextureDataComponent(l_TDC);

		GLRPC->m_GLTDCs[i] = l_TDC;

		attachRenderTargets(GLRPC, GLRPC->m_renderPassDesc.RTDesc, i);
	}

	if (GLRPC->m_drawColorBuffers)
	{
		setDrawBuffers(GLRPC->m_renderPassDesc.RTNumber);
	}
	else
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return true;
}

void GLRenderingSystemNS::generateFBO(GLRenderPassComponent* GLRPC)
{
	glGenFramebuffers(1, &GLRPC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glObjectLabel(GL_FRAMEBUFFER, GLRPC->m_FBO, (GLsizei)GLRPC->m_name.size(), GLRPC->m_name.c_str());

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: " + std::string(GLRPC->m_name.c_str()) + " FBO has been generated.");
}

void GLRenderingSystemNS::generateRBO(GLRenderPassComponent* GLRPC)
{
	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLRPC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLRPC->m_RBO);
	glObjectLabel(GL_RENDERBUFFER, GLRPC->m_RBO, (GLsizei)GLRPC->m_name.size(), GLRPC->m_name.c_str());

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GLRPC->m_renderBufferAttachmentType, GL_RENDERBUFFER, GLRPC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GLRPC->m_renderBufferInternalFormat, GLRPC->m_renderPassDesc.RTDesc.width, GLRPC->m_renderPassDesc.RTDesc.height);

	auto l_result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (l_result != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(GLRPC->m_name.c_str()) + " Framebuffer is not completed: " + std::to_string(l_result));
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: " + std::string(GLRPC->m_name.c_str()) + " RBO has been generated.");
	}
}

void GLRenderingSystemNS::attachRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex)
{
	if (RTDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		if (RTDesc.pixelDataFormat == TexturePixelDataFormat::DEPTH_COMPONENT)
		{
			bind2DDepthTextureForWrite(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC);
		}
		else
		{
			bind2DColorTextureForWrite(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC, colorAttachmentIndex);
		}
	}
	else if (RTDesc.samplerType == TextureSamplerType::SAMPLER_3D)
	{
		if (RTDesc.pixelDataFormat == TexturePixelDataFormat::DEPTH_COMPONENT)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingSystem: 3D depth only render target is not supported now");
		}
		else
		{
			bind3DColorTextureForWrite(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC, colorAttachmentIndex, 0);
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: " + std::string(GLRPC->m_name.c_str()) + " render target has been attached.");
}

void GLRenderingSystemNS::setDrawBuffers(unsigned int RTNum)
{
	std::vector<unsigned int> l_colorAttachments;
	for (unsigned int i = 0; i < RTNum; ++i)
	{
		l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);
}

bool GLRenderingSystemNS::resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, unsigned int newSizeX, unsigned int newSizeY)
{
	GLRPC->m_renderPassDesc.RTDesc.width = newSizeX;
	GLRPC->m_renderPassDesc.RTDesc.height = newSizeY;

	glDeleteFramebuffers(1, &GLRPC->m_FBO);
	generateFBO(GLRPC);

	if (GLRPC->m_renderPassDesc.useDepthAttachment)
	{
		glDeleteRenderbuffers(1, &GLRPC->m_RBO);
		generateRBO(GLRPC);
	}

	for (unsigned int i = 0; i < GLRPC->m_GLTDCs.size(); i++)
	{
		GLRPC->m_GLTDCs[i]->m_textureDataDesc.width = newSizeX;
		GLRPC->m_GLTDCs[i]->m_textureDataDesc.height = newSizeY;

		glDeleteTextures(1, &GLRPC->m_GLTDCs[i]->m_TO);

		auto l_textureDesc = GLRPC->m_GLTDCs[i]->m_textureDataDesc;

		generateTO(GLRPC->m_GLTDCs[i]->m_TO, GLRPC->m_GLTDCs[i]->m_GLTextureDataDesc, GLRPC->m_GLTDCs[i]->m_textureData);

		attachRenderTargets(GLRPC, l_textureDesc, i);
	}

	if (GLRPC->m_drawColorBuffers)
	{
		setDrawBuffers((unsigned int)GLRPC->m_GLTDCs.size());
	}
	else
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

GLShaderProgramComponent * GLRenderingSystemNS::addGLShaderProgramComponent(const EntityID& rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_GLShaderProgramComponentPool, sizeof(GLShaderProgramComponent));
	auto l_GLSPC = new(l_rawPtr)GLShaderProgramComponent();
	l_GLSPC->m_parentEntity = rhs;
	return l_GLSPC;
}

bool GLRenderingSystemNS::initializeGLMeshDataComponent(GLMeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		summitGPUData(rhs);

		rhs->m_objectStatus = ObjectStatus::Activated;

		return true;
	}
}

bool GLRenderingSystemNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingSystem: try to generate GLTextureDataComponent for TextureUsageType::INVISIBLE type!");
			return false;
		}
		else
		{
			if (rhs->m_textureData.size() > 0)
			{
				summitGPUData(rhs);

				rhs->m_objectStatus = ObjectStatus::Activated;

				if (rhs->m_textureDataDesc.usageType != TextureUsageType::COLOR_ATTACHMENT)
				{
					// @TODO: release raw data in heap memory
				}

				return true;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingSystem: try to generate GLTextureDataComponent without raw data!");
				return false;
			}
		}
	}
}

void GLRenderingSystemNS::addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath)
{
	// Create shader object
	shaderID = glCreateShader(shaderType);
	glObjectLabel(GL_SHADER, shaderID, (GLsizei)shaderFilePath.size(), shaderFilePath.c_str());

	if (shaderID == 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Shader creation failed! Memory location is invaild when add shader!");
		glDeleteShader(shaderID);
		return;
	}

	// load shader
	if (shaderFilePath.find(".spv"))
	{
		auto l_shaderCodeContent = g_pCoreSystem->getFileSystem()->loadBinaryFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()));

		if (l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " content is empty!");
			return;
		}

		// Apply the SPIR-V to the shader object.
		glShaderBinary(1, &shaderID, GL_SPIR_V_BINARY, l_shaderCodeContent.data(), (GLsizei)l_shaderCodeContent.size());

		// Specialize the shader.
		glSpecializeShader(shaderID, "main", 0, 0, 0);
	}
	else
	{
		auto l_shaderCodeContent = g_pCoreSystem->getFileSystem()->loadTextFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()));

		if (l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " content is empty!");
			return;
		}

		const char* l_sourcePointer = l_shaderCodeContent.c_str();

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

		glCompileShader(shaderID);
	}

	// Validate shader
	GLint l_validationResult = GL_FALSE;
	GLint l_infoLogLength = 0;
	GLint l_shaderFileLength = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_validationResult);

	if (!l_validationResult)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " compile failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " file length is: " + std::to_string(l_shaderFileLength));
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0)
		{
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " compile error: no info log provided!");
		}

		return;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " has been compiled.");

	// Link shader to program
	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &l_validationResult);
	if (!l_validationResult)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " link failed!");
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetProgramInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " link error: no info log provided!");
		}

		return;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: " + std::string(shaderFilePath.c_str()) + " has been linked.");
}

bool GLRenderingSystemNS::initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& ShaderFilePaths)
{
	rhs->m_program = glCreateProgram();

	if (ShaderFilePaths.m_VSPath != "")
	{
		addShader(rhs->m_program, rhs->m_VSID, GL_VERTEX_SHADER, ShaderFilePaths.m_VSPath);
	}
	if (ShaderFilePaths.m_TCSPath != "")
	{
		addShader(rhs->m_program, rhs->m_TCSID, GL_TESS_CONTROL_SHADER, ShaderFilePaths.m_TCSPath);
	}
	if (ShaderFilePaths.m_TESPath != "")
	{
		addShader(rhs->m_program, rhs->m_TESID, GL_TESS_EVALUATION_SHADER, ShaderFilePaths.m_TESPath);
	}
	if (ShaderFilePaths.m_GSPath != "")
	{
		addShader(rhs->m_program, rhs->m_GSID, GL_GEOMETRY_SHADER, ShaderFilePaths.m_GSPath);
	}
	if (ShaderFilePaths.m_FSPath != "")
	{
		addShader(rhs->m_program, rhs->m_FSID, GL_FRAGMENT_SHADER, ShaderFilePaths.m_FSPath);
	}
	if (ShaderFilePaths.m_CSPath != "")
	{
		addShader(rhs->m_program, rhs->m_CSID, GL_COMPUTE_SHADER, ShaderFilePaths.m_CSPath);
	}

	rhs->m_objectStatus = ObjectStatus::Activated;
	return rhs;
}

void GLRenderingSystemNS::generateTO(GLuint& TO, GLTextureDataDesc desc, const std::vector<void*>& textureData)
{
	glGenTextures(1, &TO);

	glBindTexture(desc.textureSamplerType, TO);

	glTexParameteri(desc.textureSamplerType, GL_TEXTURE_WRAP_R, desc.textureWrapMethod);
	glTexParameteri(desc.textureSamplerType, GL_TEXTURE_WRAP_S, desc.textureWrapMethod);
	glTexParameteri(desc.textureSamplerType, GL_TEXTURE_WRAP_T, desc.textureWrapMethod);

	glTexParameterfv(desc.textureSamplerType, GL_TEXTURE_BORDER_COLOR, desc.borderColor);

	glTexParameteri(desc.textureSamplerType, GL_TEXTURE_MIN_FILTER, desc.minFilterParam);
	glTexParameteri(desc.textureSamplerType, GL_TEXTURE_MAG_FILTER, desc.magFilterParam);

	if (desc.textureSamplerType == GL_TEXTURE_1D)
	{
		glTexImage1D(GL_TEXTURE_1D, 0, desc.internalFormat, desc.width, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, desc.internalFormat, desc.width, desc.height, desc.depth, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_CUBE_MAP)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[3]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[5]);
	}

	// should generate mipmap or not
	if (desc.minFilterParam == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(desc.textureSamplerType);
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: Texture object " + std::to_string(TO) + " is created.");
}

bool GLRenderingSystemNS::summitGPUData(GLMeshDataComponent * rhs)
{
	std::vector<float> l_verticesBuffer;
	auto l_containerSize = rhs->m_vertices.size() * 8;
	l_verticesBuffer.reserve(l_containerSize);

	std::for_each(rhs->m_vertices.begin(), rhs->m_vertices.end(), [&](Vertex val)
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

	glGenVertexArrays(1, &rhs->m_VAO);
	glBindVertexArray(rhs->m_VAO);

	glGenBuffers(1, &rhs->m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, rhs->m_VBO);

	glGenBuffers(1, &rhs->m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhs->m_IBO);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: Vertex Array Object " + std::to_string(rhs->m_VAO) + " is initialized.");

	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: Vertex Buffer Object " + std::to_string(rhs->m_VBO) + " is initialized.");

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rhs->m_indices.size() * sizeof(unsigned int), &rhs->m_indices[0], GL_STATIC_DRAW);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: Index Buffer Object " + std::to_string(rhs->m_IBO) + " is initialized.");

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedGLMDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool GLRenderingSystemNS::summitGPUData(GLTextureDataComponent * rhs)
{
	rhs->m_GLTextureDataDesc = getGLTextureDataDesc(rhs->m_textureDataDesc);

	generateTO(rhs->m_TO, rhs->m_GLTextureDataDesc, rhs->m_textureData);

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedGLTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: GLTDC " + InnoUtility::pointerToString(rhs) + " is initialized.");

	return true;
}

GLenum GLRenderingSystemNS::getTextureSamplerType(TextureSamplerType rhs)
{
	switch (rhs)
	{
	case TextureSamplerType::SAMPLER_1D:
		return GL_TEXTURE_1D;
	case TextureSamplerType::SAMPLER_2D:
		return GL_TEXTURE_2D;
	case TextureSamplerType::SAMPLER_3D:
		return GL_TEXTURE_3D;
	case TextureSamplerType::CUBEMAP:
		return GL_TEXTURE_CUBE_MAP;
	default:
		return 0;
	}
}

GLenum GLRenderingSystemNS::getTextureWrapMethod(TextureWrapMethod rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: result = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: result = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: result = GL_CLAMP_TO_BORDER; break;
	}

	return result;
}

GLenum GLRenderingSystemNS::getTextureFilterParam(TextureFilterMethod rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TextureFilterMethod::NEAREST: result = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: result = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: result = GL_LINEAR_MIPMAP_LINEAR; break;
	}

	return result;
}

GLenum GLRenderingSystemNS::getTextureInternalFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_internalFormat;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = GL_DEPTH_COMPONENT32F;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = GL_DEPTH32F_STENCIL8;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16F; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16F; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16F; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16F; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32F; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32F; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32F; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32F; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

GLenum GLRenderingSystemNS::getTexturePixelDataFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_result;

	if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8
		|| textureDataDesc.pixelDataType == TexturePixelDataType::UINT16
		|| textureDataDesc.pixelDataType == TexturePixelDataType::UINT32
		)
	{
		switch (textureDataDesc.pixelDataFormat)
		{
		case TexturePixelDataFormat::R:l_result = GL_RED_INTEGER; break;
		case TexturePixelDataFormat::RG:l_result = GL_RG_INTEGER; break;
		case TexturePixelDataFormat::RGB:l_result = GL_RGB_INTEGER; break;
		case TexturePixelDataFormat::RGBA:l_result = GL_RGBA_INTEGER; break;
		case TexturePixelDataFormat::DEPTH_COMPONENT:l_result = GL_DEPTH_COMPONENT; break;
		default:
			break;
		}
	}
	else
	{
		switch (textureDataDesc.pixelDataFormat)
		{
		case TexturePixelDataFormat::R:l_result = GL_RED; break;
		case TexturePixelDataFormat::RG:l_result = GL_RG; break;
		case TexturePixelDataFormat::RGB:l_result = GL_RGB; break;
		case TexturePixelDataFormat::RGBA:l_result = GL_RGBA; break;
		case TexturePixelDataFormat::DEPTH_COMPONENT:l_result = GL_DEPTH_COMPONENT; break;
		default:
			break;
		}
	}

	return l_result;
}

GLenum GLRenderingSystemNS::getTexturePixelDataType(TexturePixelDataType rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TexturePixelDataType::UBYTE:result = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::SBYTE:result = GL_BYTE; break;
	case TexturePixelDataType::USHORT:result = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SSHORT:result = GL_SHORT; break;
	case TexturePixelDataType::UINT8:result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT8:result = GL_INT; break;
	case TexturePixelDataType::UINT16:result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT16:result = GL_INT; break;
	case TexturePixelDataType::UINT32:result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT32:result = GL_INT; break;
	case TexturePixelDataType::FLOAT16:result = GL_HALF_FLOAT; break;
	case TexturePixelDataType::FLOAT32:result = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:result = GL_DOUBLE; break;
	}

	return result;
}

GLTextureDataDesc GLRenderingSystemNS::getGLTextureDataDesc(TextureDataDesc textureDataDesc)
{
	GLTextureDataDesc l_result;

	l_result.textureSamplerType = getTextureSamplerType(textureDataDesc.samplerType);
	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.wrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);
	l_result.internalFormat = getTextureInternalFormat(textureDataDesc);
	l_result.pixelDataFormat = getTexturePixelDataFormat(textureDataDesc);
	l_result.pixelDataType = getTexturePixelDataType(textureDataDesc.pixelDataType);
	l_result.width = textureDataDesc.width;
	l_result.height = textureDataDesc.height;
	l_result.depth = textureDataDesc.depth;
	l_result.borderColor[0] = textureDataDesc.borderColor[0];
	l_result.borderColor[1] = textureDataDesc.borderColor[1];
	l_result.borderColor[2] = textureDataDesc.borderColor[2];
	l_result.borderColor[3] = textureDataDesc.borderColor[3];

	return l_result;
}

bool GLRenderingSystemNS::deleteShaderProgram(GLShaderProgramComponent* rhs)
{
	if (rhs->m_VSID)
	{
		glDetachShader(rhs->m_program, rhs->m_VSID);
		glDeleteShader(rhs->m_VSID);
	}
	if (rhs->m_GSID)
	{
		glDetachShader(rhs->m_program, rhs->m_GSID);
		glDeleteShader(rhs->m_GSID);
	}
	if (rhs->m_FSID)
	{
		glDetachShader(rhs->m_program, rhs->m_FSID);
		glDeleteShader(rhs->m_FSID);
	}

	glDeleteProgram(rhs->m_program);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: shader deleted.");

	return true;
}

GLuint GLRenderingSystemNS::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

GLuint GLRenderingSystemNS::generateUBO(GLuint UBOSize, GLuint uniformBlockBindingPoint, const std::string& UBOName)
{
	GLuint l_UBO;
	glGenBuffers(1, &l_UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, l_UBO);
	glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, l_UBO, 0, UBOSize);
	glObjectLabel(GL_BUFFER, l_UBO, (GLsizei)UBOName.size(), UBOName.c_str());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return l_UBO;
}

void GLRenderingSystemNS::updateUBOImpl(const GLint & UBO, size_t size, const void * UBOValue)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, UBOValue);
}

bool GLRenderingSystemNS::bindUBO(const GLint& UBO, GLuint uniformBlockBindingPoint, unsigned int offset, unsigned int size)
{
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, UBO, offset, size);
	return true;
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, bool uniformValue)
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, int uniformValue)
{
	glUniform1i(uniformLocation, uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, unsigned int uniformValue)
{
	glUniform1ui(uniformLocation, uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float uniformValue)
{
	glUniform1f(uniformLocation, uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, vec2 uniformValue)
{
	glUniform2fv(uniformLocation, 1, &uniformValue.x);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, vec4 uniformValue)
{
	glUniform4fv(uniformLocation, 1, &uniformValue.x);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, const mat4 & mat)
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m00);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m00);
#endif
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, const std::vector<vec4>& uniformValue)
{
	glUniform4fv(uniformLocation, (GLsizei)uniformValue.size(), (float*)&uniformValue[0]);
}

GLuint GLRenderingSystemNS::generateSSBO(GLuint SSBOSize, GLuint bufferBlockBindingPoint, const std::string& SSBOName)
{
	GLuint l_SSBO;
	glGenBuffers(1, &l_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, l_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SSBOSize, NULL, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bufferBlockBindingPoint, l_SSBO, 0, SSBOSize);
	glObjectLabel(GL_BUFFER, l_SSBO, (GLsizei)SSBOName.size(), SSBOName.c_str());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return l_SSBO;
}

void GLRenderingSystemNS::updateSSBOImpl(const GLint & SSBO, size_t size, const void * SSBOValue)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, SSBOValue);
}

void GLRenderingSystemNS::bind2DDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingSystemNS::bindCubemapDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
}

void GLRenderingSystemNS::bind2DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingSystemNS::bind3DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int layer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_3D, GLTDC->m_TO, 0, layer);
}

void GLRenderingSystemNS::bindCubemapTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
}

void GLRenderingSystemNS::unbind2DColorTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, 0, 0);
}

void GLRenderingSystemNS::unbindCubemapTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, 0, mipLevel);
}

void GLRenderingSystemNS::activateShaderProgram(GLShaderProgramComponent * GLShaderProgramComponent)
{
	glUseProgram(GLShaderProgramComponent->m_program);
}

void GLRenderingSystemNS::drawMesh(GLMeshDataComponent* GLMDC)
{
	if (GLMDC->m_VAO)
	{
		glBindVertexArray(GLMDC->m_VAO);
		switch (GLMDC->m_meshPrimitiveTopology)
		{
		case MeshPrimitiveTopology::POINT: glDrawElements(GL_POINTS, (GLsizei)GLMDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::LINE: glDrawElements(GL_LINE, (GLsizei)GLMDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::TRIANGLE: glDrawElements(GL_TRIANGLES, (GLsizei)GLMDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::TRIANGLE_STRIP: glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)GLMDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
		default:
			break;
		}
	}
}

void GLRenderingSystemNS::activateTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureSamplerType, GLTDC->m_TO);
}

void GLRenderingSystemNS::activateRenderPass(GLRenderPassComponent* val)
{
	cleanRenderBuffers(val);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, val->m_FBO);
	if (val->m_renderPassDesc.useDepthAttachment)
	{
		glRenderbufferStorage(GL_RENDERBUFFER, val->m_renderBufferInternalFormat, val->m_renderPassDesc.RTDesc.width, val->m_renderPassDesc.RTDesc.height);
	}
	glViewport(0, 0, val->m_renderPassDesc.RTDesc.width, val->m_renderPassDesc.RTDesc.height);
};

void GLRenderingSystemNS::cleanRenderBuffers(GLRenderPassComponent* val)
{
	glBindFramebuffer(GL_FRAMEBUFFER, val->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, val->m_RBO);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);
};

void GLRenderingSystemNS::copyDepthBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyStencilBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyColorBuffer(GLRenderPassComponent* src, unsigned int srcIndex, GLRenderPassComponent* dest, unsigned int destIndex)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + srcIndex);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + destIndex);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
};

vec4 GLRenderingSystemNS::readPixel(GLRenderPassComponent* GLRPC, unsigned int colorAttachmentIndex, GLint x, GLint y)
{
	vec4 l_result;
	auto l_GLTDC = GLRPC->m_GLTDCs[colorAttachmentIndex];

	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLRPC->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + colorAttachmentIndex);
	glReadPixels(x, y, 1, 1, l_GLTDC->m_GLTextureDataDesc.pixelDataFormat, l_GLTDC->m_GLTextureDataDesc.pixelDataType, &l_result);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return l_result;
}