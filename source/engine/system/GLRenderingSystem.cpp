#include "GLRenderingSystem.h"

void GLRenderingSystem::setup()
{
	setupEnvironmentRenderPass();
}

void GLRenderingSystem::setupEnvironmentRenderPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

	// generate and bind texture
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	////
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTextureID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	////
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTextureID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	////
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: EnvironmentRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram = glCreateProgram();
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentCapturePassPBSVertex.sf");
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentCapturePassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram,
		"uni_equirectangularMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap,
		0);
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram,
		"uni_r");

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram = glCreateProgram();
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentConvolutionPassPBSVertex.sf");
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentConvolutionPassPBSFragment.sf");


	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram = glCreateProgram();
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentPreFilterPassPBSVertex.sf");
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentPreFilterPassPBSFragment.sf");

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram = glCreateProgram();
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentBRDFLUTPassPBSVertex.sf");
	setupShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentBRDFLUTPassPBSFragment.sf");
}

void GLRenderingSystem::setupGraphicPrimtives()
{
	for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
	{
		for (auto& l_graphicData : l_visibleComponent->getModelMap())
		{
			auto l_GLMesh = g_pMemorySystem->spawn<GLMeshDataComponent>();
			GLRenderingSystemSingletonComponent::getInstance().m_meshMap.emplace(l_graphicData.first, l_GLMesh);
			auto l_Mesh = g_pAssetSystem->getMesh(l_graphicData.first);
			setupMesh(l_Mesh, l_GLMesh);
			std::for_each(l_graphicData.second.begin(), l_graphicData.second.end(), [&](texturePair val) {
				auto l_GLTexture = g_pMemorySystem->spawn<GLTextureDataComponent>();
				GLRenderingSystemSingletonComponent::getInstance().m_textureMap.emplace(val.second, l_GLTexture);
				auto l_Texture = g_pAssetSystem->getTexture(val.second);
				setupTexture(l_Texture, l_GLTexture);
			});
		}
	}
}

void GLRenderingSystem::setupMesh(const MeshDataComponent * MeshDataComponent, GLMeshDataComponent* GLMeshDataComponent)
{
	GLMeshDataComponent->m_meshID = MeshDataComponent->m_meshID;
	GLMeshDataComponent->m_meshType = MeshDataComponent->m_meshType;

	glGenVertexArrays(1, &GLMeshDataComponent->m_VAO);
	glGenBuffers(1, &GLMeshDataComponent->m_VBO);
	glGenBuffers(1, &GLMeshDataComponent->m_IBO);

	std::vector<float> l_verticesBuffer;
	auto& l_vertices = MeshDataComponent->m_vertices;
	auto& l_indices = MeshDataComponent->m_indices;

	if (GLMeshDataComponent->m_meshType == meshType::TWO_DIMENSION)
	{
		std::for_each(l_vertices.begin(), l_vertices.end(), [&](Vertex val)
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
		std::for_each(l_vertices.begin(), l_vertices.end(), [&](Vertex val)
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

	glBindVertexArray(GLMeshDataComponent->m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, GLMeshDataComponent->m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLMeshDataComponent->m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_indices.size() * sizeof(unsigned int), &l_indices[0], GL_STATIC_DRAW);

	if (GLMeshDataComponent->m_meshType == meshType::TWO_DIMENSION)
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
}

void GLRenderingSystem::setupTexture(const TextureDataComponent * TextureDataComponent, GLTextureDataComponent * GLTextureDataComponent)
{
	GLTextureDataComponent->m_textureID = TextureDataComponent->m_textureID;
	GLTextureDataComponent->m_textureType = TextureDataComponent->m_textureType;

	if (GLTextureDataComponent->m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else
	{
		//generate and bind texture object
		glGenTextures(1, &GLTextureDataComponent->m_TAO);
		if (TextureDataComponent->m_textureType == textureType::CUBEMAP || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_TAO);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_TAO);
		}

		// set the texture wrapping parameters
		GLenum l_textureWrapMethod;
		switch (TextureDataComponent->m_textureWrapMethod)
		{
		case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
		case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
		case textureWrapMethod::CLAMP_TO_BORDER: l_textureWrapMethod = GL_CLAMP_TO_BORDER; break;
		}
		if (TextureDataComponent->m_textureType == textureType::CUBEMAP || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		}
		else if (TextureDataComponent->m_textureType == textureType::SHADOWMAP)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		}

		// set texture filtering parameters
		GLenum l_minFilterParam;
		switch (TextureDataComponent->m_textureMinFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_minFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_minFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		GLenum l_magFilterParam;
		switch (TextureDataComponent->m_textureMagFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_magFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_magFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		if (TextureDataComponent->m_textureType == textureType::CUBEMAP || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
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
		if (TextureDataComponent->m_textureType == textureType::ALBEDO)
		{
			if (TextureDataComponent->m_texturePixelDataFormat == texturePixelDataFormat::RGB)
			{
				l_internalFormat = GL_SRGB;
			}
			else if (TextureDataComponent->m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
			{
				l_internalFormat = GL_SRGB_ALPHA;
			}
		}
		else
		{
			switch (TextureDataComponent->m_textureColorComponentsFormat)
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
			case textureColorComponentsFormat::DEPTH_COMPONENT: l_internalFormat = GL_DEPTH_COMPONENT; break;
			}
		}
		switch (TextureDataComponent->m_texturePixelDataFormat)
		{
		case texturePixelDataFormat::RED:l_dataFormat = GL_RED; break;
		case texturePixelDataFormat::RG:l_dataFormat = GL_RG; break;
		case texturePixelDataFormat::RGB:l_dataFormat = GL_RGB; break;
		case texturePixelDataFormat::RGBA:l_dataFormat = GL_RGBA; break;
		case texturePixelDataFormat::DEPTH_COMPONENT:l_dataFormat = GL_DEPTH_COMPONENT; break;
		}
		switch (TextureDataComponent->m_texturePixelDataType)
		{
		case texturePixelDataType::UNSIGNED_BYTE:l_type = GL_UNSIGNED_BYTE; break;
		case texturePixelDataType::BYTE:l_type = GL_BYTE; break;
		case texturePixelDataType::UNSIGNED_SHORT:l_type = GL_UNSIGNED_SHORT; break;
		case texturePixelDataType::SHORT:l_type = GL_SHORT; break;
		case texturePixelDataType::UNSIGNED_INT:l_type = GL_UNSIGNED_INT; break;
		case texturePixelDataType::INT:l_type = GL_INT; break;
		case texturePixelDataType::FLOAT:l_type = GL_FLOAT; break;
		}

		if (TextureDataComponent->m_textureType == textureType::CUBEMAP || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[0]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[1]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[2]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[3]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[4]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[5]);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, TextureDataComponent->m_textureWidth, TextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, TextureDataComponent->m_textureData[0]);
		}

		// should generate mipmap or not
		if (TextureDataComponent->m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			// @TODO: generalization...
			if (TextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
			else if (TextureDataComponent->m_textureType != textureType::CUBEMAP || TextureDataComponent->m_textureType != textureType::ENVIRONMENT_CAPTURE || TextureDataComponent->m_textureType != textureType::ENVIRONMENT_CONVOLUTION || TextureDataComponent->m_textureType != textureType::RENDER_BUFFER_SAMPLER)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

		}

		m_objectStatus = objectStatus::ALIVE;
	}

}

void GLRenderingSystem::initialize()
{
}

void GLRenderingSystem::update()
{
	//// bind to framebuffer
	//glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_FBO);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glClear(GL_DEPTH_BUFFER_BIT);

	//// draw environment capture texture
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
	//glViewport(0, 0, 2048, 2048);

	//mat4 captureProjection;
	//captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0, 0.1, 10.0);
	//std::vector<mat4> captureViews =
	//{
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(-1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  1.0,  0.0, 1.0), vec4(0.0,  0.0,  1.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, -1.0,  0.0, 1.0), vec4(0.0,  0.0, -1.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0,  1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0, -1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0))
	//};

	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	//glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram);
	//updateUniform(
	//	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p,
	//	captureProjection);

	//auto& l_visibleComponents = g_pGameSystem->getVisibleComponents();
	//if (l_visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : l_visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				// activate equiretangular texture and remap equiretangular texture to cubemap
	//				auto l_equiretangularTexture = g_pAssetSystem->getTexture(textureType::EQUIRETANGULAR, l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
	//				g_pRenderingSystem->activate(l_equiretangularTexture, 0);
	//				for (unsigned int i = 0; i < 6; ++i)
	//				{
	//					updateUniform(
	//						EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r,
	//						captureViews[i]);
	//					glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID);
	//					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID, 0);
	//					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//					auto l_mesh = g_pAssetSystem->getMesh(meshType::THREE_DIMENSION, l_graphicData.first);
	//					g_pRenderingSystem->activate(l_mesh, 0);
	//					g_pRenderingSystem->draw(l_mesh);
	//				}
	//				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	//			}
	//		}
	//	}
	//}

	//// draw environment convolution texture
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	//glViewport(0, 0, 128, 128);
	//glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram);
	//updateUniform(
	//	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p,
	//	captureProjection);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
	//				auto l_environmentConvolutionTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CONVOLUTION)->second);
	//				if (l_environmentCaptureTexture != textureMap.end() && l_environmentConvolutionTexture != textureMap.end())
	//				{
	//					// @TODO: it should be update(0)?
	//					l_environmentCaptureTexture->second->update(1);
	//					for (unsigned int i = 0; i < 6; ++i)
	//					{
	//						updateUniform(m_uni_r, captureViews[i]);
	//						l_environmentConvolutionTexture->second->attachToFramebuffer(0, i, 0);
	//						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment pre-filter texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(2);
	//m_environmentPreFilterPassShaderProgram->activate();

	//m_environmentPreFilterPassShaderProgram->updateUniform(m_uni_p, captureProjection);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
	//				auto l_environmentPrefilterTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_PREFILTER)->second);
	//				if (l_environmentCaptureTexture != textureMap.end() && l_environmentPrefilterTexture != textureMap.end())
	//				{
	//					// @TODO: it should be update(0)?
	//					l_environmentCaptureTexture->second->update(2);
	//					unsigned int maxMipLevels = 5;
	//					for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	//					{
	//						// resize framebuffer according to mip-level size.
	//						unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
	//						unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

	//						glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
	//						glViewport(0, 0, mipWidth, mipHeight);

	//						double roughness = (double)mip / (double)(maxMipLevels - 1);
	//						updateUniform(m_uni_roughness, roughness);
	//						for (unsigned int i = 0; i < 6; ++i)
	//						{
	//							updateUniform(m_uni_r, captureViews[i]);
	//							l_environmentPrefilterTexture->second->attachToFramebuffer(0, i, mip);
	//							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//							meshMap.find(l_graphicData.first)->second->update();
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment BRDF look-up table texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(3);
	//m_environmentBRDFLUTPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentBRDFLUTTexture = textureMap.find(l_graphicData.second.find(textureType::RENDER_BUFFER_SAMPLER)->second);
	//				if (l_environmentBRDFLUTTexture != textureMap.end())
	//				{
	//					l_environmentBRDFLUTTexture->second->attachToFramebuffer(0, 0, 0);
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment map BRDF LUT rectangle
	//g_pRenderingSystem->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
}

void GLRenderingSystem::shutdown()
{
}

const objectStatus & GLRenderingSystem::getStatus() const
{
	return m_objectStatus;
}

void GLRenderingSystem::setupShader(GLuint shaderProgram, GLuint shaderID, GLuint shaderType, const std::string & shaderFilePath)
{
	shaderID = glCreateShader(shaderType);

	if (shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
	}

	char const * l_sourcePointer = g_pAssetSystem->loadShader(shaderFilePath).c_str();
	glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

	GLint l_compileResult = GL_FALSE;
	int l_infoLogLength = 0;

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (!l_compileResult) {
		std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
		g_pLogSystem->printLog("innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] +"\n -- --------------------------------------------------- -- ");
	}

	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	g_pLogSystem->printLog("innoShader: " + shaderFilePath + " Shader is compiled.");
}

GLuint GLRenderingSystem::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, bool uniformValue) const
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, int uniformValue) const
{
	glUniform1i(uniformLocation, uniformValue);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, double uniformValue) const
{
	glUniform1f(uniformLocation, (GLfloat)uniformValue);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, (GLfloat)x, (GLfloat)y);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, double x, double y, double z, double w) const
{
	glUniform4f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void GLRenderingSystem::updateUniform(const GLint uniformLocation, const mat4 & mat) const
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m[0][0]);
#endif
}