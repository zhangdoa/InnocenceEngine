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
	void addRenderTargetTextures(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc);
	void attachRenderTargetTextures(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex);

	void setDrawBuffers(unsigned int RTNum);

	void addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath);

	bool summitGPUData(GLMeshDataComponent* rhs);
	bool summitGPUData(GLTextureDataComponent* rhs);

	GLTextureDataDesc getGLTextureDataDesc(const TextureDataDesc& textureDataDesc);

	GLenum getTextureSamplerType(TextureSamplerType rhs);
	GLenum getTextureWrapMethod(TextureWrapMethod rhs);
	GLenum getTextureFilterParam(TextureFilterMethod rhs);
	GLenum getTextureInternalFormat(TextureColorComponentsFormat rhs);
	GLenum getTexturePixelDataFormat(TexturePixelDataFormat rhs);
	GLenum getTexturePixelDataType(TexturePixelDataType rhs);

	void generateTO(GLuint& TO, GLTextureDataDesc desc, GLsizei width, GLsizei height, GLsizei depth, const std::vector<void*>& textureData);

	std::unordered_map<EntityID, GLMeshDataComponent*> m_initializedGLMDC;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_initializedGLTDC;

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

GLRenderPassComponent* GLRenderingSystemNS::addGLRenderPassComponent(unsigned int RTNum, GLFrameBufferDesc glFrameBufferDesc, TextureDataDesc RTDesc)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_GLRenderPassComponentPool, sizeof(GLRenderPassComponent));
	auto l_GLRPC = new(l_rawPtr)GLRenderPassComponent();

	// generate and bind framebuffer
	l_GLRPC->m_GLFrameBufferDesc = glFrameBufferDesc;

	generateFBO(l_GLRPC);

	// generate and bind texture
	l_GLRPC->m_GLTDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		addRenderTargetTextures(l_GLRPC, RTDesc);
		attachRenderTargetTextures(l_GLRPC, RTDesc, i);
	}

	if (glFrameBufferDesc.drawColorBuffers)
	{
		setDrawBuffers(RTNum);
	}
	else
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	return l_GLRPC;
}

void GLRenderingSystemNS::generateFBO(GLRenderPassComponent* GLRPC)
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLRPC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLRPC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLRPC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GLRPC->m_GLFrameBufferDesc.renderBufferAttachmentType, GL_RENDERBUFFER, GLRPC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GLRPC->m_GLFrameBufferDesc.renderBufferInternalFormat, GLRPC->m_GLFrameBufferDesc.sizeX, GLRPC->m_GLFrameBufferDesc.sizeY);

	// finally check if framebuffer is complete
	auto l_result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (l_result != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Framebuffer is not completed: " + std::to_string(l_result));
	}
}

void GLRenderingSystemNS::addRenderTargetTextures(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc)
{
	auto l_TDC = addGLTextureDataComponent();

	l_TDC->m_textureDataDesc = RTDesc;

	if (RTDesc.samplerType == TextureSamplerType::CUBEMAP)
	{
		l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	}
	else
	{
		l_TDC->m_textureData = { nullptr };
	}

	initializeGLTextureDataComponent(l_TDC);

	GLRPC->m_GLTDCs.emplace_back(l_TDC);
}

void GLRenderingSystemNS::attachRenderTargetTextures(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex)
{
	if (RTDesc.samplerType == TextureSamplerType::SAMPLER_2D)
	{
		if (RTDesc.pixelDataFormat == TexturePixelDataFormat::DEPTH_COMPONENT)
		{
			attach2DDepthRT(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC);
		}
		else
		{
			attach2DColorRT(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC, colorAttachmentIndex);
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
			attach3DColorRT(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC, colorAttachmentIndex, 0);
		}
	}
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
	GLRPC->m_GLFrameBufferDesc.sizeX = newSizeX;
	GLRPC->m_GLFrameBufferDesc.sizeY = newSizeY;

	glDeleteFramebuffers(1, &GLRPC->m_FBO);
	glDeleteRenderbuffers(1, &GLRPC->m_RBO);

	generateFBO(GLRPC);

	for (unsigned int i = 0; i < GLRPC->m_GLTDCs.size(); i++)
	{
		GLRPC->m_GLTDCs[i]->m_textureDataDesc.width = newSizeX;
		GLRPC->m_GLTDCs[i]->m_textureDataDesc.height = newSizeY;

		glDeleteTextures(1, &GLRPC->m_GLTDCs[i]->m_TO);

		auto l_textureDesc = GLRPC->m_GLTDCs[i]->m_textureDataDesc;

		generateTO(GLRPC->m_GLTDCs[i]->m_TO, GLRPC->m_GLTDCs[i]->m_GLTextureDataDesc, l_textureDesc.width, l_textureDesc.height, l_textureDesc.depth, GLRPC->m_GLTDCs[i]->m_textureData);

		attachRenderTargetTextures(GLRPC, l_textureDesc, i);
	}

	if (GLRPC->m_GLFrameBufferDesc.drawColorBuffers)
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
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return true;
	}
	else
	{
		summitGPUData(rhs);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return true;
	}
}

bool GLRenderingSystemNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
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

				rhs->m_objectStatus = ObjectStatus::ALIVE;

				if (rhs->m_textureDataDesc.usageType != TextureUsageType::RENDER_TARGET)
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

void GLRenderingSystemNS::addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath)
{
	shaderID = glCreateShader(shaderType);

	if (shaderID == 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Shader creation failed! Memory location invaild when adding shader!");
		glDeleteShader(shaderID);
		return;
	}

	if (shaderFilePath.find(".spv") != std::string::npos)
	{
		auto l_shaderCodeContent = g_pCoreSystem->getFileSystem()->loadBinaryFile(m_shaderRelativePath + shaderFilePath);

		if (l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " loading failed!");
			return;
		}

		// Apply the SPIR-V to the shader object.
		glShaderBinary(1, &shaderID, GL_SPIR_V_BINARY, l_shaderCodeContent.data(), (GLsizei)l_shaderCodeContent.size());

		// Specialize the shader.
		glSpecializeShader(shaderID, "main", 0, 0, 0);
	}
	else
	{
		auto l_shaderCodeContent = g_pCoreSystem->getFileSystem()->loadTextFile(m_shaderRelativePath + shaderFilePath);

		if (l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " loading failed!");
			return;
		}

		const char* l_sourcePointer = l_shaderCodeContent.c_str();

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

		// compile shader
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is compiling ...");

		glCompileShader(shaderID);
	}

	GLint l_validationResult = GL_FALSE;
	GLint l_infoLogLength = 0;
	GLint l_shaderFileLength = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_validationResult);

	if (!l_validationResult)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " compile failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0)
		{
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " compile error: no info log provided!");
		}

		return;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " has been compiled.");

	// Link shader to program
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is linking ...");

	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &l_validationResult);
	if (!l_validationResult)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " link failed!");
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetProgramInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " link error: no info log provided!");
		}

		return;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " has been linked.");
}

bool GLRenderingSystemNS::initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& ShaderFilePaths)
{
	rhs->m_program = glCreateProgram();

	if (!ShaderFilePaths.m_VSPath.empty())
	{
		addShader(rhs->m_program, rhs->m_VSID, GL_VERTEX_SHADER, ShaderFilePaths.m_VSPath);
	}
	if (!ShaderFilePaths.m_GSPath.empty())
	{
		addShader(rhs->m_program, rhs->m_GSID, GL_GEOMETRY_SHADER, ShaderFilePaths.m_GSPath);
	}
	if (!ShaderFilePaths.m_FSPath.empty())
	{
		addShader(rhs->m_program, rhs->m_FSID, GL_FRAGMENT_SHADER, ShaderFilePaths.m_FSPath);
	}
	if (!ShaderFilePaths.m_CSPath.empty())
	{
		addShader(rhs->m_program, rhs->m_CSID, GL_COMPUTE_SHADER, ShaderFilePaths.m_CSPath);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;
	return rhs;
}

void GLRenderingSystemNS::generateTO(GLuint& TO, GLTextureDataDesc desc, GLsizei width, GLsizei height, GLsizei depth, const std::vector<void*>& textureData)
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
		glTexImage1D(GL_TEXTURE_1D, 0, desc.internalFormat, width, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, desc.internalFormat, width, height, depth, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_CUBE_MAP)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[3]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, desc.internalFormat, width, height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData[5]);
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

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedGLMDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool GLRenderingSystemNS::summitGPUData(GLTextureDataComponent * rhs)
{
	rhs->m_GLTextureDataDesc = getGLTextureDataDesc(rhs->m_textureDataDesc);

	generateTO(rhs->m_TO, rhs->m_GLTextureDataDesc, rhs->m_textureDataDesc.width, rhs->m_textureDataDesc.height, rhs->m_textureDataDesc.depth, rhs->m_textureData);

	rhs->m_objectStatus = ObjectStatus::ALIVE;

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

GLenum GLRenderingSystemNS::getTextureInternalFormat(TextureColorComponentsFormat rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TextureColorComponentsFormat::RED: result = GL_RED; break;
	case TextureColorComponentsFormat::RG: result = GL_RG; break;
	case TextureColorComponentsFormat::RGB: result = GL_RGB; break;
	case TextureColorComponentsFormat::RGBA: result = GL_RGBA; break;

	case TextureColorComponentsFormat::R8: result = GL_R8; break;
	case TextureColorComponentsFormat::RG8: result = GL_RG8; break;
	case TextureColorComponentsFormat::RGB8: result = GL_RGB8; break;
	case TextureColorComponentsFormat::RGBA8: result = GL_RGBA8; break;

	case TextureColorComponentsFormat::R8I: result = GL_R8I; break;
	case TextureColorComponentsFormat::RG8I: result = GL_RG8I; break;
	case TextureColorComponentsFormat::RGB8I: result = GL_RGB8I; break;
	case TextureColorComponentsFormat::RGBA8I: result = GL_RGBA8I; break;

	case TextureColorComponentsFormat::R8UI: result = GL_R8UI; break;
	case TextureColorComponentsFormat::RG8UI: result = GL_RG8UI; break;
	case TextureColorComponentsFormat::RGB8UI: result = GL_RGB8UI; break;
	case TextureColorComponentsFormat::RGBA8UI: result = GL_RGBA8UI; break;

	case TextureColorComponentsFormat::R16: result = GL_R16; break;
	case TextureColorComponentsFormat::RG16: result = GL_RG16; break;
	case TextureColorComponentsFormat::RGB16: result = GL_RGB16; break;
	case TextureColorComponentsFormat::RGBA16: result = GL_RGBA16; break;

	case TextureColorComponentsFormat::R16I: result = GL_R16I; break;
	case TextureColorComponentsFormat::RG16I: result = GL_RG16I; break;
	case TextureColorComponentsFormat::RGB16I: result = GL_RGB16I; break;
	case TextureColorComponentsFormat::RGBA16I: result = GL_RGBA16I; break;

	case TextureColorComponentsFormat::R16UI: result = GL_R16UI; break;
	case TextureColorComponentsFormat::RG16UI: result = GL_RG16UI; break;
	case TextureColorComponentsFormat::RGB16UI: result = GL_RGB16UI; break;
	case TextureColorComponentsFormat::RGBA16UI: result = GL_RGBA16UI; break;

	case TextureColorComponentsFormat::R16F: result = GL_R16F; break;
	case TextureColorComponentsFormat::RG16F: result = GL_RG16F; break;
	case TextureColorComponentsFormat::RGB16F: result = GL_RGB16F; break;
	case TextureColorComponentsFormat::RGBA16F: result = GL_RGBA16F; break;

	case TextureColorComponentsFormat::R32I: result = GL_R32I; break;
	case TextureColorComponentsFormat::RG32I: result = GL_RG32I; break;
	case TextureColorComponentsFormat::RGB32I: result = GL_RGB32I; break;
	case TextureColorComponentsFormat::RGBA32I: result = GL_RGBA32I; break;

	case TextureColorComponentsFormat::R32UI: result = GL_R32UI; break;
	case TextureColorComponentsFormat::RG32UI: result = GL_RG32UI; break;
	case TextureColorComponentsFormat::RGB32UI: result = GL_RGB32UI; break;
	case TextureColorComponentsFormat::RGBA32UI: result = GL_RGBA32UI; break;

	case TextureColorComponentsFormat::R32F: result = GL_R32F; break;
	case TextureColorComponentsFormat::RG32F: result = GL_RG32F; break;
	case TextureColorComponentsFormat::RGB32F: result = GL_RGB32F; break;
	case TextureColorComponentsFormat::RGBA32F: result = GL_RGBA32F; break;

	case TextureColorComponentsFormat::SRGB: result = GL_SRGB; break;
	case TextureColorComponentsFormat::SRGBA: result = GL_SRGB_ALPHA; break;
	case TextureColorComponentsFormat::SRGB8: result = GL_SRGB8; break;
	case TextureColorComponentsFormat::SRGBA8: result = GL_SRGB8_ALPHA8; break;

	case TextureColorComponentsFormat::DEPTH_COMPONENT: result = GL_DEPTH_COMPONENT; break;
	}

	return result;
}

GLenum GLRenderingSystemNS::getTexturePixelDataFormat(TexturePixelDataFormat rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TexturePixelDataFormat::RED:result = GL_RED; break;
	case TexturePixelDataFormat::RG:result = GL_RG; break;
	case TexturePixelDataFormat::RGB:result = GL_RGB; break;
	case TexturePixelDataFormat::RGBA:result = GL_RGBA; break;
	case TexturePixelDataFormat::RED_INT:result = GL_RED_INTEGER; break;
	case TexturePixelDataFormat::RG_INT:result = GL_RG_INTEGER; break;
	case TexturePixelDataFormat::RGB_INT:result = GL_RGB_INTEGER; break;
	case TexturePixelDataFormat::RGBA_INT:result = GL_RGBA_INTEGER; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:result = GL_DEPTH_COMPONENT; break;
	default:
		break;
	}

	return result;
}

GLenum GLRenderingSystemNS::getTexturePixelDataType(TexturePixelDataType rhs)
{
	GLenum result;

	switch (rhs)
	{
	case TexturePixelDataType::UNSIGNED_BYTE:result = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::BYTE:result = GL_BYTE; break;
	case TexturePixelDataType::UNSIGNED_SHORT:result = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SHORT:result = GL_SHORT; break;
	case TexturePixelDataType::UNSIGNED_INT:result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::INT:result = GL_INT; break;
	case TexturePixelDataType::FLOAT:result = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:result = GL_FLOAT; break;
	}

	return result;
}

GLTextureDataDesc GLRenderingSystemNS::getGLTextureDataDesc(const TextureDataDesc& textureDataDesc)
{
	GLTextureDataDesc l_result;

	l_result.textureSamplerType = getTextureSamplerType(textureDataDesc.samplerType);

	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.wrapMethod);

	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);

	// set texture formats
	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_result.internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_result.internalFormat = GL_SRGB_ALPHA;
		}
	}
	else
	{
		l_result.internalFormat = getTextureInternalFormat(textureDataDesc.colorComponentsFormat);
	}

	l_result.pixelDataFormat = getTexturePixelDataFormat(textureDataDesc.pixelDataFormat);

	l_result.pixelDataType = getTexturePixelDataType(textureDataDesc.pixelDataType);

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

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: shader deleted.");

	return true;
}

GLuint GLRenderingSystemNS::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

GLuint GLRenderingSystemNS::getUniformBlockIndex(GLuint shaderProgram, const std::string & uniformBlockName)
{
	glUseProgram(shaderProgram);
	auto uniformBlockIndex = glGetUniformBlockIndex(shaderProgram, uniformBlockName.c_str());
	if (uniformBlockIndex == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Uniform Block lost: " + uniformBlockName);
		return -1;
	}
	return uniformBlockIndex;
}

GLuint GLRenderingSystemNS::generateUBO(GLuint UBOSize, GLuint uniformBlockBindingPoint)
{
	GLuint l_UBO;
	glGenBuffers(1, &l_UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, l_UBO);
	glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, l_UBO, 0, UBOSize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return l_UBO;
}

void GLRenderingSystemNS::bindUniformBlock(GLuint UBO, GLuint UBOSize, GLuint program, const std::string & uniformBlockName, GLuint uniformBlockBindingPoint)
{
	auto uniformBlockIndex = getUniformBlockIndex(program, uniformBlockName.c_str());
	glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBindingPoint);
}

void GLRenderingSystemNS::updateTextureUniformLocations(GLuint program, const std::vector<std::string>& UniformNames)
{
	for (size_t i = 0; i < UniformNames.size(); i++)
	{
		updateUniform(getUniformLocation(program, UniformNames[i]), (int)i);
	}
}

void GLRenderingSystemNS::updateUBOImpl(const GLint & UBO, size_t size, const void * UBOValue)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, UBOValue);
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

void GLRenderingSystemNS::attach2DDepthRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingSystemNS::attachCubemapDepthRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
}

void GLRenderingSystemNS::attach2DColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingSystemNS::attach3DColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int layer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_3D, GLTDC->m_TO, 0, layer);
}

void GLRenderingSystemNS::attachCubemapColorRT(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
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
	glRenderbufferStorage(GL_RENDERBUFFER, val->m_GLFrameBufferDesc.renderBufferInternalFormat, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
	glViewport(0, 0, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
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
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyStencilBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyColorBuffer(GLRenderPassComponent* src, unsigned int srcIndex, GLRenderPassComponent* dest, unsigned int destIndex)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + srcIndex);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + destIndex);
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_COLOR_BUFFER_BIT, GL_LINEAR);
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