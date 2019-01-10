#include "GLRenderingSystemUtilities.h"
#include "../component/GLRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	std::unordered_map<EntityID, GLMeshDataComponent*> m_initializedGLMDC;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_initializedGLTDC;

	std::unordered_map<EntityID, GLMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_textureMap;

	const std::string m_shaderRelativePath = std::string{ "..//res//shaders//" };
}

GLRenderPassComponent* GLRenderingSystemNS::addGLRenderPassComponent(unsigned int RTNum, GLFrameBufferDesc glFrameBufferDesc, TextureDataDesc RTDesc)
{
	auto l_GLRPC = g_pCoreSystem->getMemorySystem()->spawn<GLRenderPassComponent>();

	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();
	l_GLRPC->m_GLFBC = l_FBC;

	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX = glFrameBufferDesc.sizeX;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY = glFrameBufferDesc.sizeY;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferAttachmentType = glFrameBufferDesc.renderBufferAttachmentType;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferInternalFormat = glFrameBufferDesc.renderBufferInternalFormat;

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferAttachmentType, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferInternalFormat, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY);

	// generate and bind texture
	l_GLRPC->m_TDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureDataDesc.textureUsageType = RTDesc.textureUsageType;
		l_TDC->m_textureDataDesc.textureColorComponentsFormat = RTDesc.textureColorComponentsFormat;
		l_TDC->m_textureDataDesc.texturePixelDataFormat = RTDesc.texturePixelDataFormat;
		l_TDC->m_textureDataDesc.textureMinFilterMethod = RTDesc.textureMinFilterMethod;
		l_TDC->m_textureDataDesc.textureMagFilterMethod = RTDesc.textureMagFilterMethod;
		l_TDC->m_textureDataDesc.textureWrapMethod = RTDesc.textureWrapMethod;
		l_TDC->m_textureDataDesc.textureWidth = RTDesc.textureWidth;
		l_TDC->m_textureDataDesc.textureHeight = RTDesc.textureHeight;
		l_TDC->m_textureDataDesc.texturePixelDataType = RTDesc.texturePixelDataType;
		if (RTDesc.textureUsageType == TextureUsageType::CUBEMAP)
		{
			l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		}
		else
		{
			l_TDC->m_textureData = { nullptr };
		}

		l_GLRPC->m_TDCs.emplace_back(l_TDC);
	}

	l_GLRPC->m_GLTDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = l_GLRPC->m_TDCs[i];
		auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

		if (!(RTDesc.textureUsageType == TextureUsageType::CUBEMAP))
		{
			attachTextureToFramebuffer(
				l_TDC,
				l_GLTDC,
				l_FBC,
				(int)i, 0, 0
			);
		}

		l_GLRPC->m_GLTDCs.emplace_back(l_GLTDC);
	}

	if (glFrameBufferDesc.drawColorBuffers)
	{
		std::vector<unsigned int> l_colorAttachments;
		for (unsigned int i = 0; i < RTNum; ++i)
		{
			l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);
	}
	else
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return l_GLRPC;
}

void GLRenderingSystemNS::addRenderTargetTextures(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, int colorAttachmentIndex, int textureIndex, int mipLevel)
{
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = RTDesc;
	l_TDC->m_textureData = { nullptr };

	GLRPC->m_TDCs.emplace_back(l_TDC);

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		GLRPC->m_GLFBC,
		colorAttachmentIndex, textureIndex, mipLevel
	);

	GLRPC->m_GLTDCs.emplace_back(l_GLTDC);
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

bool GLRenderingSystemNS::resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, GLFrameBufferDesc glFrameBufferDesc)
{
	GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX = glFrameBufferDesc.sizeX;
	GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY = glFrameBufferDesc.sizeY;

	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_GLFBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLRPC->m_GLFBC->m_RBO);

	for (unsigned int i = 0; i < GLRPC->m_GLTDCs.size(); i++)
	{
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureWidth = GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX;
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureHeight = GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY;
		GLRPC->m_TDCs[i]->m_objectStatus = ObjectStatus::STANDBY;
		glDeleteTextures(1, &GLRPC->m_GLTDCs[i]->m_TAO);
		initializeGLTextureDataComponent(GLRPC->m_GLTDCs[i], GLRPC->m_TDCs[i]->m_textureDataDesc, GLRPC->m_TDCs[i]->m_textureData);
		attachTextureToFramebuffer(
			GLRPC->m_TDCs[i],
			GLRPC->m_GLTDCs[i],
			GLRPC->m_GLFBC,
			(int)i, 0, 0
		);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

GLShaderProgramComponent * GLRenderingSystemNS::addGLShaderProgramComponent(const EntityID& rhs)
{
	GLShaderProgramComponent* newShaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();
	newShaderProgram->m_parentEntity = rhs;
	return newShaderProgram;
}

GLMeshDataComponent * GLRenderingSystemNS::addGLMeshDataComponent(const EntityID& rhs)
{
	GLMeshDataComponent* newMesh = g_pCoreSystem->getMemorySystem()->spawn<GLMeshDataComponent>();
	newMesh->m_parentEntity = rhs;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, GLMeshDataComponent*>(rhs, newMesh));
	return newMesh;
}

GLTextureDataComponent * GLRenderingSystemNS::addGLTextureDataComponent(const EntityID& rhs)
{
	GLTextureDataComponent* newTexture = g_pCoreSystem->getMemorySystem()->spawn<GLTextureDataComponent>();
	newTexture->m_parentEntity = rhs;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, GLTextureDataComponent*>(rhs, newTexture));
	return newTexture;
}

GLMeshDataComponent * GLRenderingSystemNS::getGLMeshDataComponent(const EntityID& rhs)
{
	auto result = m_meshMap.find(rhs);
	if (result != m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

GLTextureDataComponent * GLRenderingSystemNS::getGLTextureDataComponent(const EntityID& rhs)
{
	auto result = m_textureMap.find(rhs);
	if (result != m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

GLMeshDataComponent* GLRenderingSystemNS::generateGLMeshDataComponent(MeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getGLMeshDataComponent(rhs->m_parentEntity);
	}
	else
	{
		auto l_ptr = addGLMeshDataComponent(rhs->m_parentEntity);

		initializeGLMeshDataComponent(l_ptr, rhs->m_vertices, rhs->m_indices);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return l_ptr;
	}
}

GLTextureDataComponent* GLRenderingSystemNS::generateGLTextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getGLTextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureDataDesc.textureUsageType == TextureUsageType::INVISIBLE)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingSystem: try to generate GLTextureDataComponent for TextureUsageType::INVISIBLE type!");
			return nullptr;
		}
		else
		{
			if (rhs->m_textureData.size() > 0)
			{
				auto l_ptr = addGLTextureDataComponent(rhs->m_parentEntity);

				initializeGLTextureDataComponent(l_ptr, rhs->m_textureDataDesc, rhs->m_textureData);

				rhs->m_objectStatus = ObjectStatus::ALIVE;

				g_pCoreSystem->getAssetSystem()->releaseRawDataForTextureDataComponent(rhs->m_parentEntity);

				return l_ptr;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingSystem: try to generate GLTextureDataComponent without raw data!");
				return nullptr;
			}
		}
	}
}

bool GLRenderingSystemNS::initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& ShaderFilePaths)
{
	rhs->m_program = glCreateProgram();

	std::function<void(GLuint& shaderProgram, GLuint& shaderID, GLuint ShaderType, const std::string & shaderFilePath)> f_addShader =
		[&](GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath) {
		shaderID = glCreateShader(shaderType);

		if (shaderID == 0)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Shader creation failed! memory location invaild when adding shader!");
			return;
		}

		auto l_shaderCodeContent = g_pCoreSystem->getFileSystem()->loadTextFile(m_shaderRelativePath + shaderFilePath);
		const char* l_sourcePointer = l_shaderCodeContent.c_str();

		if (l_sourcePointer == nullptr || l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " loading failed!");
			return;
		}

		// compile shader
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is compiling ...");

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);
		glCompileShader(shaderID);

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

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been compiled.");

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

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been linked.");
	};

	if (!ShaderFilePaths.m_VSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_VSID, GL_VERTEX_SHADER, ShaderFilePaths.m_VSPath);
	}
	if (!ShaderFilePaths.m_GSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_GSID, GL_GEOMETRY_SHADER, ShaderFilePaths.m_GSPath);
	}
	if (!ShaderFilePaths.m_FSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_FSID, GL_FRAGMENT_SHADER, ShaderFilePaths.m_FSPath);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;
	return rhs;
}

bool GLRenderingSystemNS::initializeGLMeshDataComponent(GLMeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices)
{
	std::vector<float> l_verticesBuffer;
	auto l_containerSize = vertices.size() * 8;
	l_verticesBuffer.reserve(l_containerSize);

	std::for_each(vertices.begin(), vertices.end(), [&](Vertex val)
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
	glGenBuffers(1, &rhs->m_VBO);
	glGenBuffers(1, &rhs->m_IBO);

	glBindVertexArray(rhs->m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, rhs->m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhs->m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedGLMDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: VAO " + std::to_string(rhs->m_VAO) + " is initialized.");

	return true;
}

bool GLRenderingSystemNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
{
	rhs->m_GLTextureDataDesc = getGLTextureDataDesc(textureDataDesc);

	//generate and bind texture object
	glGenTextures(1, &rhs->m_TAO);

	glBindTexture(rhs->m_GLTextureDataDesc.textureType, rhs->m_TAO);

	if (rhs->m_GLTextureDataDesc.textureType == GL_TEXTURE_CUBE_MAP)
	{
		glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_R, rhs->m_GLTextureDataDesc.textureWrapMethod);
	}
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_S, rhs->m_GLTextureDataDesc.textureWrapMethod);
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_T, rhs->m_GLTextureDataDesc.textureWrapMethod);

	glTexParameterfv(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_BORDER_COLOR, rhs->m_GLTextureDataDesc.boardColor);

	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_MIN_FILTER, rhs->m_GLTextureDataDesc.minFilterParam);
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_MAG_FILTER, rhs->m_GLTextureDataDesc.magFilterParam);

	if (rhs->m_GLTextureDataDesc.textureType == GL_TEXTURE_CUBE_MAP)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[3]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[5]);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[0]);
	}

	// should generate mipmap or not
	if (rhs->m_GLTextureDataDesc.minFilterParam == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(rhs->m_GLTextureDataDesc.textureType);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedGLTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: TAO " + std::to_string(rhs->m_TAO) + " is initialized.");

	return true;
}

GLTextureDataDesc GLRenderingSystemNS::getGLTextureDataDesc(const TextureDataDesc& textureDataDesc)
{
	GLTextureDataDesc l_result;

	// texture type
	if (textureDataDesc.textureUsageType == TextureUsageType::CUBEMAP)
	{
		l_result.textureType = GL_TEXTURE_CUBE_MAP;
	}
	else
	{
		l_result.textureType = GL_TEXTURE_2D;
	}

	// set the texture wrapping parameters
	switch (textureDataDesc.textureWrapMethod)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: l_result.textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: l_result.textureWrapMethod = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: l_result.textureWrapMethod = GL_CLAMP_TO_BORDER; break;
	}

	// set texture filtering parameters
	switch (textureDataDesc.textureMinFilterMethod)
	{
	case TextureFilterMethod::NEAREST: l_result.minFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_result.minFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_result.minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;
	}
	switch (textureDataDesc.textureMagFilterMethod)
	{
	case TextureFilterMethod::NEAREST: l_result.magFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_result.magFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_result.magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;
	}

	// set texture formats
	if (textureDataDesc.textureUsageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_result.internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_result.internalFormat = GL_SRGB_ALPHA;
		}
	}
	else
	{
		switch (textureDataDesc.textureColorComponentsFormat)
		{
		case TextureColorComponentsFormat::RED: l_result.internalFormat = GL_RED; break;
		case TextureColorComponentsFormat::RG: l_result.internalFormat = GL_RG; break;
		case TextureColorComponentsFormat::RGB: l_result.internalFormat = GL_RGB; break;
		case TextureColorComponentsFormat::RGBA: l_result.internalFormat = GL_RGBA; break;
		case TextureColorComponentsFormat::R8: l_result.internalFormat = GL_R8; break;
		case TextureColorComponentsFormat::RG8: l_result.internalFormat = GL_RG8; break;
		case TextureColorComponentsFormat::RGB8: l_result.internalFormat = GL_RGB8; break;
		case TextureColorComponentsFormat::RGBA8: l_result.internalFormat = GL_RGBA8; break;
		case TextureColorComponentsFormat::R16: l_result.internalFormat = GL_R16; break;
		case TextureColorComponentsFormat::RG16: l_result.internalFormat = GL_RG16; break;
		case TextureColorComponentsFormat::RGB16: l_result.internalFormat = GL_RGB16; break;
		case TextureColorComponentsFormat::RGBA16: l_result.internalFormat = GL_RGBA16; break;
		case TextureColorComponentsFormat::R16F: l_result.internalFormat = GL_R16F; break;
		case TextureColorComponentsFormat::RG16F: l_result.internalFormat = GL_RG16F; break;
		case TextureColorComponentsFormat::RGB16F: l_result.internalFormat = GL_RGB16F; break;
		case TextureColorComponentsFormat::RGBA16F: l_result.internalFormat = GL_RGBA16F; break;
		case TextureColorComponentsFormat::R32F: l_result.internalFormat = GL_R32F; break;
		case TextureColorComponentsFormat::RG32F: l_result.internalFormat = GL_RG32F; break;
		case TextureColorComponentsFormat::RGB32F: l_result.internalFormat = GL_RGB32F; break;
		case TextureColorComponentsFormat::RGBA32F: l_result.internalFormat = GL_RGBA32F; break;
		case TextureColorComponentsFormat::SRGB: l_result.internalFormat = GL_SRGB; break;
		case TextureColorComponentsFormat::SRGBA: l_result.internalFormat = GL_SRGB_ALPHA; break;
		case TextureColorComponentsFormat::SRGB8: l_result.internalFormat = GL_SRGB8; break;
		case TextureColorComponentsFormat::SRGBA8: l_result.internalFormat = GL_SRGB8_ALPHA8; break;
		case TextureColorComponentsFormat::DEPTH_COMPONENT: l_result.internalFormat = GL_DEPTH_COMPONENT; break;
		}
	}
	switch (textureDataDesc.texturePixelDataFormat)
	{
	case TexturePixelDataFormat::RED:l_result.pixelDataFormat = GL_RED; break;
	case TexturePixelDataFormat::RG:l_result.pixelDataFormat = GL_RG; break;
	case TexturePixelDataFormat::RGB:l_result.pixelDataFormat = GL_RGB; break;
	case TexturePixelDataFormat::RGBA:l_result.pixelDataFormat = GL_RGBA; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:l_result.pixelDataFormat = GL_DEPTH_COMPONENT; break;
	}
	switch (textureDataDesc.texturePixelDataType)
	{
	case TexturePixelDataType::UNSIGNED_BYTE:l_result.pixelDataType = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::BYTE:l_result.pixelDataType = GL_BYTE; break;
	case TexturePixelDataType::UNSIGNED_SHORT:l_result.pixelDataType = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SHORT:l_result.pixelDataType = GL_SHORT; break;
	case TexturePixelDataType::UNSIGNED_INT:l_result.pixelDataType = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::INT:l_result.pixelDataType = GL_INT; break;
	case TexturePixelDataType::FLOAT:l_result.pixelDataType = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:l_result.pixelDataType = GL_FLOAT; break;
	}

	l_result.boardColor[0] = textureDataDesc.borderColor.x;
	l_result.boardColor[1] = textureDataDesc.borderColor.y;
	l_result.boardColor[2] = textureDataDesc.borderColor.z;
	l_result.boardColor[3] = textureDataDesc.borderColor.w;

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
	// @TODO: UBO
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

GLuint GLRenderingSystemNS::generateUBO(GLuint UBOSize)
{
	GLuint l_ubo;
	glGenBuffers(1, &l_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, l_ubo);
	glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return l_ubo;
}

void GLRenderingSystemNS::bindUniformBlock(GLuint UBO, GLuint UBOSize, GLuint program, const std::string & uniformBlockName, GLuint uniformBlockBindingPoint)
{
	auto uniformBlockIndex = getUniformBlockIndex(program, uniformBlockName.c_str());
	glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBindingPoint);
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, UBO, 0, UBOSize);
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
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, bool uniformValue)
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, int uniformValue)
{
	glUniform1i(uniformLocation, uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float uniformValue)
{
	glUniform1f(uniformLocation, (GLfloat)uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y)
{
	glUniform2f(uniformLocation, (GLfloat)x, (GLfloat)y);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y, float z)
{
	glUniform3f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y, float z, float w)
{
	glUniform4f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
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

void GLRenderingSystemNS::attachTextureToFramebuffer(TextureDataComponent * TDC, GLTextureDataComponent * GLTDC, GLFrameBufferComponent * GLFBC, int colorAttachmentIndex, int textureIndex, int mipLevel)
{
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureType, GLTDC->m_TAO);
	if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::CUBEMAP)
	{
		if (TDC->m_textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::DEPTH_COMPONENT)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TAO, mipLevel);
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TAO, mipLevel);
		}
	}
	else
	{
		if (TDC->m_textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::DEPTH_COMPONENT)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TAO, mipLevel);
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTDC->m_TAO, mipLevel);
		}		
	}
}

void GLRenderingSystemNS::activateShaderProgram(GLShaderProgramComponent * GLShaderProgramComponent)
{
	glUseProgram(GLShaderProgramComponent->m_program);
}

void GLRenderingSystemNS::drawMesh(const EntityID& rhs)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(rhs);
	if (l_MDC)
	{
		drawMesh(l_MDC);
	}
}

void GLRenderingSystemNS::drawMesh(MeshDataComponent* MDC)
{
	auto l_GLMDC = getGLMeshDataComponent(MDC->m_parentEntity);
	if (l_GLMDC)
	{
		drawMesh(MDC->m_indicesSize, MDC->m_meshDrawMethod, l_GLMDC);
	}
}

void GLRenderingSystemNS::drawMesh(size_t indicesSize, MeshPrimitiveTopology MeshPrimitiveTopology, GLMeshDataComponent* GLMDC)
{
	if (GLMDC->m_VAO)
	{
		glBindVertexArray(GLMDC->m_VAO);
		switch (MeshPrimitiveTopology)
		{
		case MeshPrimitiveTopology::POINT: glDrawElements(GL_POINTS, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::LINE: glDrawElements(GL_LINE, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::TRIANGLE: glDrawElements(GL_TRIANGLES, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::TRIANGLE_STRIP: glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		default:
			break;
		}
	}
}

void GLRenderingSystemNS::activateTexture(TextureDataComponent * TDC, int activateIndex)
{
	auto l_GLTDC = getGLTextureDataComponent(TDC->m_parentEntity);
	activateTexture(l_GLTDC, activateIndex);
}

void GLRenderingSystemNS::activateTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureType, GLTDC->m_TAO);
}

void GLRenderingSystemNS::bindFBC(GLFrameBufferComponent* val) {
	cleanFBC(val);

	glRenderbufferStorage(GL_RENDERBUFFER, val->m_GLFrameBufferDesc.renderBufferInternalFormat, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
	glViewport(0, 0, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
};

void GLRenderingSystemNS::cleanFBC(GLFrameBufferComponent* val) {
	glBindFramebuffer(GL_FRAMEBUFFER, val->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, val->m_RBO);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);
};

void GLRenderingSystemNS::copyDepthBuffer(GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyStencilBuffer(GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingSystemNS::copyColorBuffer(GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
};