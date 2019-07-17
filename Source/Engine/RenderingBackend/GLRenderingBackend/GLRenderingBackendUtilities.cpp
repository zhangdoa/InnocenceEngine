#include "GLRenderingBackendUtilities.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE GLRenderingBackendNS
{
	void getGLError()
	{
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(err));
		}
	}

	void generateFBO(GLRenderPassComponent* GLRPC);
	void generateRBO(GLRenderPassComponent* GLRPC);

	void addRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc);
	void attachRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex);

	void setDrawBuffers(unsigned int RTNum);

	std::string loadShaderFile(const std::string & path);
	void addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath);

	bool submitGPUData(GLMeshDataComponent* rhs);

	GLTextureDataDesc getGLTextureDataDesc(TextureDataDesc textureDataDesc);
	GLenum getTextureSamplerType(TextureSamplerType rhs);
	GLenum getTextureWrapMethod(TextureWrapMethod rhs);
	GLenum getTextureFilterParam(TextureFilterMethod rhs);
	GLenum getTextureInternalFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataFormat(TextureDataDesc textureDataDesc);
	GLenum getTexturePixelDataType(TexturePixelDataType rhs);
	GLsizei getTexturePixelDataSize(TextureDataDesc textureDataDesc);

	void generateTO(GLuint& TO, GLTextureDataDesc desc, const void* textureData);

	bool submitGPUData(GLTextureDataComponent* rhs);

	bool submitGPUData(GLMaterialDataComponent* rhs);

	std::unordered_map<InnoEntity*, GLMeshDataComponent*> m_initializedGLMeshes;
	std::unordered_map<InnoEntity*, GLTextureDataComponent*> m_initializedGLTextures;
	std::unordered_map<InnoEntity*, GLMaterialDataComponent*> m_initializedGLMaterials;

	void* m_GLRenderPassComponentPool;
	void* m_GLShaderProgramComponentPool;

	const std::string m_shaderRelativePath = "Res//Shaders//GL//";
}

bool GLRenderingBackendNS::initializeComponentPool()
{
	m_GLRenderPassComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(GLRenderPassComponent), 128);
	m_GLShaderProgramComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(GLShaderProgramComponent), 256);

	return true;
}

GLRenderPassComponent* GLRenderingBackendNS::addGLRenderPassComponent(const EntityID& parentEntity, const char* name)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_GLRenderPassComponentPool, sizeof(GLRenderPassComponent));
	auto l_GLRPC = new(l_rawPtr)GLRenderPassComponent();
	l_GLRPC->m_parentEntity = parentEntity;
	l_GLRPC->m_name = name;

	return l_GLRPC;
}

bool GLRenderingBackendNS::initializeGLRenderPassComponent(GLRenderPassComponent* GLRPC)
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

		l_TDC->m_textureData = nullptr;

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

void GLRenderingBackendNS::generateFBO(GLRenderPassComponent* GLRPC)
{
	glGenFramebuffers(1, &GLRPC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glObjectLabel(GL_FRAMEBUFFER, GLRPC->m_FBO, (GLsizei)GLRPC->m_name.size(), GLRPC->m_name.c_str());

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: " + std::string(GLRPC->m_name.c_str()) + " FBO has been generated.");
}

void GLRenderingBackendNS::generateRBO(GLRenderPassComponent* GLRPC)
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
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(GLRPC->m_name.c_str()) + " Framebuffer is not completed: " + std::to_string(l_result));
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: " + std::string(GLRPC->m_name.c_str()) + " RBO has been generated.");
	}
}

void GLRenderingBackendNS::attachRenderTargets(GLRenderPassComponent* GLRPC, TextureDataDesc RTDesc, unsigned int colorAttachmentIndex)
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
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingBackend: 3D depth only render target is not supported now");
		}
		else
		{
			bind3DColorTextureForWrite(GLRPC->m_GLTDCs[colorAttachmentIndex], GLRPC, colorAttachmentIndex, 0);
		}
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: " + std::string(GLRPC->m_name.c_str()) + " render target has been attached.");
}

void GLRenderingBackendNS::setDrawBuffers(unsigned int RTNum)
{
	std::vector<unsigned int> l_colorAttachments;
	for (unsigned int i = 0; i < RTNum; ++i)
	{
		l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);
}

bool GLRenderingBackendNS::resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, unsigned int newSizeX, unsigned int newSizeY)
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
		glDeleteTextures(1, &GLRPC->m_GLTDCs[i]->m_TO);

		GLRPC->m_GLTDCs[i]->m_textureDataDesc.width = newSizeX;
		GLRPC->m_GLTDCs[i]->m_textureDataDesc.height = newSizeY;

		GLRPC->m_GLTDCs[i]->m_GLTextureDataDesc = getGLTextureDataDesc(GLRPC->m_GLTDCs[i]->m_textureDataDesc);

		generateTO(GLRPC->m_GLTDCs[i]->m_TO, GLRPC->m_GLTDCs[i]->m_GLTextureDataDesc, GLRPC->m_GLTDCs[i]->m_textureData);

		attachRenderTargets(GLRPC, GLRPC->m_GLTDCs[i]->m_textureDataDesc, i);
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

GLShaderProgramComponent * GLRenderingBackendNS::addGLShaderProgramComponent(const EntityID& rhs)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_GLShaderProgramComponentPool, sizeof(GLShaderProgramComponent));
	auto l_GLSPC = new(l_rawPtr)GLShaderProgramComponent();
	l_GLSPC->m_parentEntity = rhs;
	return l_GLSPC;
}

bool GLRenderingBackendNS::initializeGLMeshDataComponent(GLMeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		submitGPUData(rhs);

		return true;
	}
}

bool GLRenderingBackendNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingBackend: try to generate GLTextureDataComponent for TextureUsageType::INVISIBLE type!");
			return false;
		}
		else if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
		{
			submitGPUData(rhs);
			return true;
		}
		else
		{
			if (rhs->m_textureData)
			{
				submitGPUData(rhs);

				return true;
			}
			else
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "GLRenderingBackend: try to generate GLTextureDataComponent without raw data!");
				return false;
			}
		}
	}
}

std::string GLRenderingBackendNS::loadShaderFile(const std::string & path)
{
	auto f_findIncludeFilePath = [](const std::string & content) {
		auto l_includePos = content.find("#include ");
		return l_includePos;
	};

	auto f_findGLSLExtensionPos = [](const std::string & content) {
		size_t l_glslExtensionPos = std::string::npos;

		l_glslExtensionPos = content.find(".glsl");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".vert");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".tesc");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".tese");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".geom");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".frag");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".comp");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		return l_glslExtensionPos;
	};

	auto l_content = g_pModuleManager->getFileSystem()->loadTextFile(m_shaderRelativePath + path);

	auto l_includePos = f_findIncludeFilePath(l_content);

	while (l_includePos != std::string::npos)
	{
		auto l_GLSLExtensionPos = f_findGLSLExtensionPos(l_content);
		auto l_includedFileName = l_content.substr(l_includePos + 10, l_GLSLExtensionPos - 5 - l_includePos);
		l_content.replace(l_includePos, l_GLSLExtensionPos - l_includePos + 6, loadShaderFile(l_includedFileName));

		l_includePos = f_findIncludeFilePath(l_content);
	}

	return l_content;
}

void GLRenderingBackendNS::addShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const ShaderFilePath& shaderFilePath)
{
	// Create shader object
	shaderID = glCreateShader(shaderType);
	glObjectLabel(GL_SHADER, shaderID, (GLsizei)shaderFilePath.size(), shaderFilePath.c_str());

	if (shaderID == 0)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: Shader creation failed! Memory location is invaild when add shader!");
		glDeleteShader(shaderID);
		return;
	}

	// load shader
	if (shaderFilePath.find(".spv"))
	{
		auto l_shaderCodeContent = g_pModuleManager->getFileSystem()->loadBinaryFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()));

		if (l_shaderCodeContent.empty())
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " content is empty!");
			return;
		}

		// Apply the SPIR-V to the shader object.
		glShaderBinary(1, &shaderID, GL_SPIR_V_BINARY, l_shaderCodeContent.data(), (GLsizei)l_shaderCodeContent.size());

		// Specialize the shader.
		glSpecializeShader(shaderID, "main", 0, 0, 0);
	}
	else
	{
		auto l_shaderCodeContent = loadShaderFile(std::string(shaderFilePath.c_str()));

		if (l_shaderCodeContent.empty())
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " content is empty!");
			return;
		}

#ifdef _DEBUG
		std::ofstream m_debugExportFile;
		m_debugExportFile.open(g_pModuleManager->getFileSystem()->getWorkingDirectory() + "Res//Intermediate//Shaders//GL//" + std::string(shaderFilePath.c_str()) + ".gen");
		m_debugExportFile << l_shaderCodeContent;
		m_debugExportFile.close();
#endif // _DEBUG

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
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " compile failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " file length is: " + std::to_string(l_shaderFileLength));
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0)
		{
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " compile error: no info log provided!");
		}

		return;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " has been compiled.");

	// Link shader to program
	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &l_validationResult);
	if (!l_validationResult)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " link failed!");
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetProgramInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " link error: no info log provided!");
		}

		return;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: " + std::string(shaderFilePath.c_str()) + " has been linked.");
}

bool GLRenderingBackendNS::initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& ShaderFilePaths)
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

void GLRenderingBackendNS::generateTO(GLuint& TO, GLTextureDataDesc desc, const void* textureData)
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
		glTexImage1D(GL_TEXTURE_1D, 0, desc.internalFormat, desc.width, 0, desc.pixelDataFormat, desc.pixelDataType, textureData);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, desc.internalFormat, desc.width, desc.height, desc.depth, 0, desc.pixelDataFormat, desc.pixelDataType, textureData);
	}
	else if (desc.textureSamplerType == GL_TEXTURE_CUBE_MAP)
	{
		if (textureData)
		{
			for (unsigned int i = 0; i < 6; i++)
			{
				char* l_textureData = reinterpret_cast<char*>(const_cast<void*>(textureData));
				auto l_offset = i * desc.width * desc.height * desc.pixelDataSize;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, l_textureData + l_offset);
			}
		}
		else
		{
			for (unsigned int i = 0; i < 6; i++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, desc.internalFormat, desc.width, desc.height, 0, desc.pixelDataFormat, desc.pixelDataType, textureData);
			}
		}
	}

	// should generate mipmap or not
	if (desc.minFilterParam == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(desc.textureSamplerType);
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Texture object " + std::to_string(TO) + " is created.");
}

bool GLRenderingBackendNS::submitGPUData(GLMeshDataComponent * rhs)
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

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Vertex Array Object " + std::to_string(rhs->m_VAO) + " is initialized.");

	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Vertex Buffer Object " + std::to_string(rhs->m_VBO) + " is initialized.");

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rhs->m_indices.size() * sizeof(unsigned int), &rhs->m_indices[0], GL_STATIC_DRAW);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Index Buffer Object " + std::to_string(rhs->m_IBO) + " is initialized.");

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedGLMeshes.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool GLRenderingBackendNS::submitGPUData(GLTextureDataComponent * rhs)
{
	rhs->m_GLTextureDataDesc = getGLTextureDataDesc(rhs->m_textureDataDesc);

	generateTO(rhs->m_TO, rhs->m_GLTextureDataDesc, rhs->m_textureData);

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedGLTextures.emplace(rhs->m_parentEntity, rhs);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: GLTDC " + InnoUtility::pointerToString(rhs) + " is initialized.");

	return true;
}

GLenum GLRenderingBackendNS::getTextureSamplerType(TextureSamplerType rhs)
{
	switch (rhs)
	{
	case TextureSamplerType::SAMPLER_1D:
		return GL_TEXTURE_1D;
	case TextureSamplerType::SAMPLER_2D:
		return GL_TEXTURE_2D;
	case TextureSamplerType::SAMPLER_3D:
		return GL_TEXTURE_3D;
	case TextureSamplerType::SAMPLER_CUBEMAP:
		return GL_TEXTURE_CUBE_MAP;
	default:
		return 0;
	}
}

GLenum GLRenderingBackendNS::getTextureWrapMethod(TextureWrapMethod rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: l_result = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: l_result = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: l_result = GL_CLAMP_TO_BORDER; break;
	}

	return l_result;
}

GLenum GLRenderingBackendNS::getTextureFilterParam(TextureFilterMethod rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TextureFilterMethod::NEAREST: l_result = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_result = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_result = GL_LINEAR_MIPMAP_LINEAR; break;
	}

	return l_result;
}

GLenum GLRenderingBackendNS::getTextureInternalFormat(TextureDataDesc textureDataDesc)
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

GLenum GLRenderingBackendNS::getTexturePixelDataFormat(TextureDataDesc textureDataDesc)
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

GLenum GLRenderingBackendNS::getTexturePixelDataType(TexturePixelDataType rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TexturePixelDataType::UBYTE:l_result = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::SBYTE:l_result = GL_BYTE; break;
	case TexturePixelDataType::USHORT:l_result = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SSHORT:l_result = GL_SHORT; break;
	case TexturePixelDataType::UINT8:l_result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT8:l_result = GL_INT; break;
	case TexturePixelDataType::UINT16:l_result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT16:l_result = GL_INT; break;
	case TexturePixelDataType::UINT32:l_result = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::SINT32:l_result = GL_INT; break;
	case TexturePixelDataType::FLOAT16:l_result = GL_HALF_FLOAT; break;
	case TexturePixelDataType::FLOAT32:l_result = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:l_result = GL_DOUBLE; break;
	}

	return l_result;
}

GLsizei GLRenderingBackendNS::getTexturePixelDataSize(TextureDataDesc textureDataDesc)
{
	GLsizei l_singlePixelSize;

	switch (textureDataDesc.pixelDataType)
	{
	case TexturePixelDataType::UBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::USHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SSHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::SINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::FLOAT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::FLOAT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::DOUBLE:l_singlePixelSize = 8; break;
	}

	GLsizei l_channelSize;
	switch (textureDataDesc.pixelDataFormat)
	{
	case TexturePixelDataFormat::R:l_channelSize = 1; break;
	case TexturePixelDataFormat::RG:l_channelSize = 2; break;
	case TexturePixelDataFormat::RGB:l_channelSize = 3; break;
	case TexturePixelDataFormat::RGBA:l_channelSize = 4; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:l_channelSize = 1; break;
	case TexturePixelDataFormat::DEPTH_STENCIL_COMPONENT:l_channelSize = 1; break;
	default:
		break;
	}

	return l_singlePixelSize * l_channelSize;
}

GLTextureDataDesc GLRenderingBackendNS::getGLTextureDataDesc(TextureDataDesc textureDataDesc)
{
	GLTextureDataDesc l_result;

	l_result.textureSamplerType = getTextureSamplerType(textureDataDesc.samplerType);
	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.wrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);
	l_result.internalFormat = getTextureInternalFormat(textureDataDesc);
	l_result.pixelDataFormat = getTexturePixelDataFormat(textureDataDesc);
	l_result.pixelDataType = getTexturePixelDataType(textureDataDesc.pixelDataType);
	l_result.pixelDataSize = getTexturePixelDataSize(textureDataDesc);
	l_result.width = textureDataDesc.width;
	l_result.height = textureDataDesc.height;
	l_result.depth = textureDataDesc.depth;
	l_result.borderColor[0] = textureDataDesc.borderColor[0];
	l_result.borderColor[1] = textureDataDesc.borderColor[1];
	l_result.borderColor[2] = textureDataDesc.borderColor[2];
	l_result.borderColor[3] = textureDataDesc.borderColor[3];

	return l_result;
}

bool GLRenderingBackendNS::initializeGLMaterialDataComponent(GLMaterialDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		submitGPUData(rhs);

		return true;
	}
}

bool GLRenderingBackendNS::submitGPUData(GLMaterialDataComponent * rhs)
{
	if (rhs->m_normalTexture)
	{
		initializeGLTextureDataComponent(reinterpret_cast<GLTextureDataComponent*>(rhs->m_normalTexture));
	}
	if (rhs->m_albedoTexture)
	{
		initializeGLTextureDataComponent(reinterpret_cast<GLTextureDataComponent*>(rhs->m_albedoTexture));
	}
	if (rhs->m_metallicTexture)
	{
		initializeGLTextureDataComponent(reinterpret_cast<GLTextureDataComponent*>(rhs->m_metallicTexture));
	}
	if (rhs->m_roughnessTexture)
	{
		initializeGLTextureDataComponent(reinterpret_cast<GLTextureDataComponent*>(rhs->m_roughnessTexture));
	}
	if (rhs->m_aoTexture)
	{
		initializeGLTextureDataComponent(reinterpret_cast<GLTextureDataComponent*>(rhs->m_aoTexture));
	}

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedGLMaterials.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool GLRenderingBackendNS::deleteShaderProgram(GLShaderProgramComponent* rhs)
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

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: shader deleted.");

	return true;
}

GLuint GLRenderingBackendNS::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

GLuint GLRenderingBackendNS::generateUBO(GLuint UBOSize, GLuint uniformBlockBindingPoint, const std::string& UBOName)
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

void GLRenderingBackendNS::updateUBOImpl(const GLint & UBO, size_t size, const void * UBOValue)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, UBOValue);
}

bool GLRenderingBackendNS::bindUBO(const GLint& UBO, GLuint uniformBlockBindingPoint, unsigned int offset, unsigned int size)
{
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, UBO, offset, size);
	return true;
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, bool uniformValue)
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, int uniformValue)
{
	glUniform1i(uniformLocation, uniformValue);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, unsigned int uniformValue)
{
	glUniform1ui(uniformLocation, uniformValue);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, float uniformValue)
{
	glUniform1f(uniformLocation, uniformValue);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, vec2 uniformValue)
{
	glUniform2fv(uniformLocation, 1, &uniformValue.x);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, vec4 uniformValue)
{
	glUniform4fv(uniformLocation, 1, &uniformValue.x);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, const mat4 & mat)
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m00);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m00);
#endif
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, const std::vector<vec4>& uniformValue)
{
	glUniform4fv(uniformLocation, (GLsizei)uniformValue.size(), (float*)&uniformValue[0]);
}

void GLRenderingBackendNS::updateUniform(const GLint uniformLocation, const std::vector<mat4>& uniformValue)
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, (GLsizei)uniformValue.size(), GL_FALSE, (float*)&uniformValue[0]);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, (GLsizei)uniformValue.size(), GL_TRUE, (float*)&uniformValue[0]);
#endif
}

GLuint GLRenderingBackendNS::generateSSBO(GLuint SSBOSize, GLuint bufferBlockBindingPoint, const std::string& SSBOName)
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

void GLRenderingBackendNS::updateSSBOImpl(const GLint & SSBO, size_t size, const void * SSBOValue)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, SSBOValue);
}

void GLRenderingBackendNS::bind2DDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingBackendNS::bindCubemapDepthTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
}

void GLRenderingBackendNS::bind2DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTDC->m_TO, 0);
}

void GLRenderingBackendNS::bind3DColorTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int layer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_3D, GLTDC->m_TO, 0, layer);
}

void GLRenderingBackendNS::bindCubemapTextureForWrite(GLTextureDataComponent * GLTDC, GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
}

void GLRenderingBackendNS::unbind2DColorTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, 0, 0);
}

void GLRenderingBackendNS::unbindCubemapTextureForWrite(GLRenderPassComponent * GLRPC, unsigned int colorAttachmentIndex, unsigned int textureIndex, unsigned int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, 0, mipLevel);
}

void GLRenderingBackendNS::activateShaderProgram(GLShaderProgramComponent * GLShaderProgramComponent)
{
	glUseProgram(GLShaderProgramComponent->m_program);
}

void GLRenderingBackendNS::drawMesh(GLMeshDataComponent* GLMDC)
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

void GLRenderingBackendNS::activateTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureSamplerType, GLTDC->m_TO);
}

void GLRenderingBackendNS::activateRenderPass(GLRenderPassComponent* val)
{
	cleanRenderBuffers(val);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, val->m_FBO);
	if (val->m_renderPassDesc.useDepthAttachment)
	{
		glRenderbufferStorage(GL_RENDERBUFFER, val->m_renderBufferInternalFormat, val->m_renderPassDesc.RTDesc.width, val->m_renderPassDesc.RTDesc.height);
	}
	glViewport(0, 0, val->m_renderPassDesc.RTDesc.width, val->m_renderPassDesc.RTDesc.height);
};

void GLRenderingBackendNS::cleanRenderBuffers(GLRenderPassComponent* val)
{
	glBindFramebuffer(GL_FRAMEBUFFER, val->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, val->m_RBO);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);
};

void GLRenderingBackendNS::copyDepthBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingBackendNS::copyStencilBuffer(GLRenderPassComponent* src, GLRenderPassComponent* dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
};

void GLRenderingBackendNS::copyColorBuffer(GLRenderPassComponent* src, unsigned int srcIndex, GLRenderPassComponent* dest, unsigned int destIndex)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + srcIndex);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + destIndex);
	glBlitFramebuffer(0, 0, src->m_renderPassDesc.RTDesc.width, src->m_renderPassDesc.RTDesc.height, 0, 0, dest->m_renderPassDesc.RTDesc.width, dest->m_renderPassDesc.RTDesc.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
};

vec4 GLRenderingBackendNS::readPixel(GLRenderPassComponent* GLRPC, unsigned int colorAttachmentIndex, GLint x, GLint y)
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