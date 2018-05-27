#include "GLRenderingSystem.h"

void GLRenderingSystem::setup()
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

void GLRenderingSystem::initialize()
{
}

void GLRenderingSystem::update()
{
	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_FBO);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	// draw environment capture texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
	glViewport(0, 0, 2048, 2048);

	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0, 0.1, 10.0);
	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(-1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  1.0,  0.0, 1.0), vec4(0.0,  0.0,  1.0, 0.0)),
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, -1.0,  0.0, 1.0), vec4(0.0,  0.0, -1.0, 0.0)),
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0,  1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
		mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0, -1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0))
	};

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram);
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p,
		captureProjection);

	auto& l_visibleComponents = g_pGameSystem->getVisibleComponents();
	if (l_visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : l_visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					// activate equiretangular texture and remap equiretangular texture to cubemap
					auto l_equiretangularTexture = g_pRenderingSystem->m_textureMap.find(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
					l_equiretangularTexture->second->activate(0);
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(
							EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r,
							captureViews[i]);
						glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTextureID, 0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						g_pRenderingSystem->m_meshMap.find(l_graphicData.first)->second->update();
					}
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				}
			}
		}
	}

	// draw environment convolution texture
	m_environmentPassFrameBuffer->setRenderBufferStorageSize(1);
	m_environmentConvolutionPassShaderProgram->activate();

	m_environmentConvolutionPassShaderProgram->updateUniform(m_uni_p, captureProjection);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
					auto l_environmentConvolutionTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CONVOLUTION)->second);
					if (l_environmentCaptureTexture != textureMap.end() && l_environmentConvolutionTexture != textureMap.end())
					{
						// @TODO: it should be update(0)?
						l_environmentCaptureTexture->second->update(1);
						for (unsigned int i = 0; i < 6; ++i)
						{
							updateUniform(m_uni_r, captureViews[i]);
							l_environmentConvolutionTexture->second->attachToFramebuffer(0, i, 0);
							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							meshMap.find(l_graphicData.first)->second->update();
						}
					}
				}
			}
		}
	}

	// draw environment pre-filter texture
	m_environmentPassFrameBuffer->setRenderBufferStorageSize(2);
	m_environmentPreFilterPassShaderProgram->activate();

	m_environmentPreFilterPassShaderProgram->updateUniform(m_uni_p, captureProjection);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
					auto l_environmentPrefilterTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_PREFILTER)->second);
					if (l_environmentCaptureTexture != textureMap.end() && l_environmentPrefilterTexture != textureMap.end())
					{
						// @TODO: it should be update(0)?
						l_environmentCaptureTexture->second->update(2);
						unsigned int maxMipLevels = 5;
						for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
						{
							// resize framebuffer according to mip-level size.
							unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
							unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

							glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
							glViewport(0, 0, mipWidth, mipHeight);

							double roughness = (double)mip / (double)(maxMipLevels - 1);
							updateUniform(m_uni_roughness, roughness);
							for (unsigned int i = 0; i < 6; ++i)
							{
								updateUniform(m_uni_r, captureViews[i]);
								l_environmentPrefilterTexture->second->attachToFramebuffer(0, i, mip);
								glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
								meshMap.find(l_graphicData.first)->second->update();
							}
						}
					}
				}
			}
		}
	}

	// draw environment BRDF look-up table texture
	m_environmentPassFrameBuffer->setRenderBufferStorageSize(3);
	m_environmentBRDFLUTPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentBRDFLUTTexture = textureMap.find(l_graphicData.second.find(textureType::RENDER_BUFFER_SAMPLER)->second);
					if (l_environmentBRDFLUTTexture != textureMap.end())
					{
						l_environmentBRDFLUTTexture->second->attachToFramebuffer(0, 0, 0);
					}
				}
			}
		}
	}

	// draw environment map BRDF LUT rectangle
	g_pRenderingSystem->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
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