#include "GLRenderingSystem.h"

void GLRenderingSystem::setup()
{
	// 16x antialiasing
	glfwWindowHint(GLFW_SAMPLES, 16);
	// MSAA
	glEnable(GL_MULTISAMPLE);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_TEXTURE_2D);
}

void GLRenderingSystem::initialize()
{
	initializeEnvironmentRenderPass();
	initializeShadowRenderPass();
	initializeDefaultGraphicPrimtives();
	initializeGraphicPrimtivesOfComponents();
}

void GLRenderingSystem::initializeEnvironmentRenderPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

	// generate and bind texture
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_TAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_TAO);

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
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_TAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_TAO);

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
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_TAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_TAO);

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
	glGenTextures(1, &EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_TAO);
	glBindTexture(GL_TEXTURE_2D, EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: EnvironmentRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program = glCreateProgram();
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentCapturePassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentCapturePassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		"uni_equirectangularMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap,
		0);
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		"uni_r");

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program = glCreateProgram();
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentConvolutionPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentConvolutionPassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_p = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_r = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		"uni_r");

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program = glCreateProgram();
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentPreFilterPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentPreFilterPassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_roughness = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		"uni_roughness");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_p = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_r = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		"uni_r");

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram.m_program = glCreateProgram();
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/environmentBRDFLUTPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/environmentBRDFLUTPassPBSFragment.sf");
}

void GLRenderingSystem::initializeShadowRenderPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

	// generate and bind texture
	glGenTextures(1, &ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L0.m_TAO);
	glBindTexture(GL_TEXTURE_2D, ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L0.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_ATTACHMENT, 2048, 2048, 0, GL_DEPTH_ATTACHMENT, GL_FLOAT, nullptr);

	glGenTextures(1, &ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L1.m_TAO);
	glBindTexture(GL_TEXTURE_2D, ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L1.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_ATTACHMENT, 2048, 2048, 0, GL_DEPTH_ATTACHMENT, GL_FLOAT, nullptr);

	glGenTextures(1, &ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L2.m_TAO);
	glBindTexture(GL_TEXTURE_2D, ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L2.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_ATTACHMENT, 2048, 2048, 0, GL_DEPTH_ATTACHMENT, GL_FLOAT, nullptr);

	glGenTextures(1, &ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L3.m_TAO);
	glBindTexture(GL_TEXTURE_2D, ShadowRenderPassSingletonComponent::getInstance().m_shadowForwardPassTextureID_L3.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_ATTACHMENT, 2048, 2048, 0, GL_DEPTH_ATTACHMENT, GL_FLOAT, nullptr);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: ShadowRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program = glCreateProgram();
	initializeShader(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/shadowPassVertex.sf");
	initializeShader(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/shadowPassFragment.sf");

	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p = getUniformLocation(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		"uni_p");
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v = getUniformLocation(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		"uni_v");
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m = getUniformLocation(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		"uni_m");
}

void GLRenderingSystem::initializeGeometryRenderPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

	// generate and bind texture
	glGenTextures(1, &GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT0.m_TAO);
	glBindTexture(GL_TEXTURE_2D, GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT0.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, nullptr);

	glGenTextures(1, &GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT1.m_TAO);
	glBindTexture(GL_TEXTURE_2D, GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT1.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, nullptr);

	glGenTextures(1, &GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT2.m_TAO);
	glBindTexture(GL_TEXTURE_2D, GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT2.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, nullptr);

	glGenTextures(1, &GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT3.m_TAO);
	glBindTexture(GL_TEXTURE_2D, GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextureID_RT3.m_TAO);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, nullptr);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: ShadowRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program = glCreateProgram();
#ifdef CookTorrance
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/geometryCookTorrancePassVertex.sf");
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/geometryCookTorrancePassFragment.sf");
#elif BlinnPhong
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3/geometryBlinnPhongPassVertex.sf");
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3/geometryBlinnPhongPassFragment.sf");
#endif
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_r");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_t");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_m");
}

void GLRenderingSystem::initializeLightRenderPass()
{
}

void GLRenderingSystem::initializeFinalRenderPass()
{
}

void GLRenderingSystem::initializeShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath)
{
	shaderID = glCreateShader(shaderType);

	if (shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
	}

	auto l_shaderCodeContent = g_pAssetSystem->loadShader(shaderFilePath);
	const char* l_sourcePointer = l_shaderCodeContent.c_str();

	glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

	GLint l_compileResult = GL_FALSE;
	int l_infoLogLength = 0;
	int l_shaderFileLength = 0;
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &l_compileResult);
	glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);
	glGetShaderiv(shaderProgram, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
	
	if (l_infoLogLength > 0) {
		std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
		g_pLogSystem->printLog("innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
	}

	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	g_pLogSystem->printLog("innoShader: compiling " + shaderFilePath + " ...");

	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaderID, 1024, NULL, infoLog);
		g_pLogSystem->printLog("innoShader: compile error: " + std::string(infoLog) + "\n -- --------------------------------------------------- -- ");
	}

	g_pLogSystem->printLog("innoShader: " + shaderFilePath + " Shader is compiled.");
}

void GLRenderingSystem::initializeDefaultGraphicPrimtives()
{
	initializeMesh(g_pAssetSystem->getDefaultMesh(meshShapeType::LINE));
	initializeMesh(g_pAssetSystem->getDefaultMesh(meshShapeType::QUAD));
	initializeMesh(g_pAssetSystem->getDefaultMesh(meshShapeType::CUBE));
	initializeMesh(g_pAssetSystem->getDefaultMesh(meshShapeType::SPHERE));

	initializeTexture(g_pAssetSystem->getDefaultTexture(textureType::NORMAL));
	initializeTexture(g_pAssetSystem->getDefaultTexture(textureType::ALBEDO));
	initializeTexture(g_pAssetSystem->getDefaultTexture(textureType::METALLIC));
	initializeTexture(g_pAssetSystem->getDefaultTexture(textureType::ROUGHNESS));
	initializeTexture(g_pAssetSystem->getDefaultTexture(textureType::AMBIENT_OCCLUSION));
}

void GLRenderingSystem::initializeGraphicPrimtivesOfComponents()
{
	for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
	{
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				if (GLRenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.find(l_graphicData.first) == GLRenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.end())
				{
					auto l_Mesh = g_pAssetSystem->getMesh(l_graphicData.first);
					initializeMesh(l_Mesh);
					std::for_each(l_graphicData.second.begin(), l_graphicData.second.end(), [&](texturePair val) {
						auto l_Texture = g_pAssetSystem->getTexture(val.second);
						initializeTexture(l_Texture);
						GLRenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(val.second, l_Texture);
					});
					GLRenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_graphicData.first, l_Mesh);
				}
			}
	}
}

void GLRenderingSystem::initializeMesh(MeshDataComponent* GLMeshDataComponent)
{
	glGenVertexArrays(1, &GLMeshDataComponent->m_VAO);
	glGenBuffers(1, &GLMeshDataComponent->m_VBO);
	glGenBuffers(1, &GLMeshDataComponent->m_IBO);

	std::vector<float> l_verticesBuffer;
	auto& l_vertices = GLMeshDataComponent->m_vertices;
	auto& l_indices = GLMeshDataComponent->m_indices;

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

	glBindVertexArray(GLMeshDataComponent->m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, GLMeshDataComponent->m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLMeshDataComponent->m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_indices.size() * sizeof(unsigned int), &l_indices[0], GL_STATIC_DRAW);

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

void GLRenderingSystem::initializeTexture(TextureDataComponent * GLTextureDataComponent)
{
	if (GLTextureDataComponent->m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else
	{
		//generate and bind texture object
		glGenTextures(1, &GLTextureDataComponent->m_TAO);
		if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_TAO);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_TAO);
		}

		// set the texture wrapping parameters
		GLenum l_textureWrapMethod;
		switch (GLTextureDataComponent->m_textureWrapMethod)
		{
		case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
		case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
		case textureWrapMethod::CLAMP_TO_BORDER: l_textureWrapMethod = GL_CLAMP_TO_BORDER; break;
		}
		if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		}
		else if (GLTextureDataComponent->m_textureType == textureType::SHADOWMAP)
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
		switch (GLTextureDataComponent->m_textureMinFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_minFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_minFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		GLenum l_magFilterParam;
		switch (GLTextureDataComponent->m_textureMagFilterMethod)
		{
		case textureFilterMethod::NEAREST: l_magFilterParam = GL_NEAREST; break;
		case textureFilterMethod::LINEAR: l_magFilterParam = GL_LINEAR; break;
		case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

		}
		if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
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
		if (GLTextureDataComponent->m_textureType == textureType::ALBEDO)
		{
			if (GLTextureDataComponent->m_texturePixelDataFormat == texturePixelDataFormat::RGB)
			{
				l_internalFormat = GL_SRGB;
			}
			else if (GLTextureDataComponent->m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
			{
				l_internalFormat = GL_SRGB_ALPHA;
			}
		}
		else
		{
			switch (GLTextureDataComponent->m_textureColorComponentsFormat)
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
		switch (GLTextureDataComponent->m_texturePixelDataFormat)
		{
		case texturePixelDataFormat::RED:l_dataFormat = GL_RED; break;
		case texturePixelDataFormat::RG:l_dataFormat = GL_RG; break;
		case texturePixelDataFormat::RGB:l_dataFormat = GL_RGB; break;
		case texturePixelDataFormat::RGBA:l_dataFormat = GL_RGBA; break;
		case texturePixelDataFormat::DEPTH_COMPONENT:l_dataFormat = GL_DEPTH_COMPONENT; break;
		}
		switch (GLTextureDataComponent->m_texturePixelDataType)
		{
		case texturePixelDataType::UNSIGNED_BYTE:l_type = GL_UNSIGNED_BYTE; break;
		case texturePixelDataType::BYTE:l_type = GL_BYTE; break;
		case texturePixelDataType::UNSIGNED_SHORT:l_type = GL_UNSIGNED_SHORT; break;
		case texturePixelDataType::SHORT:l_type = GL_SHORT; break;
		case texturePixelDataType::UNSIGNED_INT:l_type = GL_UNSIGNED_INT; break;
		case texturePixelDataType::INT:l_type = GL_INT; break;
		case texturePixelDataType::FLOAT:l_type = GL_FLOAT; break;
		}

		if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[0]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[1]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[2]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[3]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[4]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[5]);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, GLTextureDataComponent->m_textureWidth, GLTextureDataComponent->m_textureHeight, 0, l_dataFormat, l_type, GLTextureDataComponent->m_textureData[0]);
		}

		// should generate mipmap or not
		if (GLTextureDataComponent->m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
		{
			// @TODO: generalization...
			if (GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
			else if (GLTextureDataComponent->m_textureType != textureType::CUBEMAP || GLTextureDataComponent->m_textureType != textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType != textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType != textureType::RENDER_BUFFER_SAMPLER)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}
		}
	}

}

void GLRenderingSystem::update()
{
	if (GLRenderingSystemSingletonComponent::getInstance().m_shouldUpdateEnvironmentMap)
	{
		updateEnvironmentRenderPass();
		GLRenderingSystemSingletonComponent::getInstance().m_shouldUpdateEnvironmentMap = false;
	}
	updateShadowRenderPass();
}

void GLRenderingSystem::updateEnvironmentRenderPass()
{
	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
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

	glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program);
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
					auto l_equiretangularTexture = g_pAssetSystem->getTexture(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
					activateTexture(l_equiretangularTexture, 0);
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(
							EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r,
							captureViews[i]);
						glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_TAO);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_TAO, 0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
						activateMesh(l_mesh);
						drawMesh(l_mesh);
					}
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				}
			}
		}
	}

	// draw environment convolution texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program);
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_p,
		captureProjection);

	if (l_visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : l_visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture;
					auto l_environmentConvolutionTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture;
					activateTexture(l_environmentCaptureTexture, 1);
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(
							EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_r,
							captureViews[i]);
						attachTextureToFramebuffer(l_environmentConvolutionTexture, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent, 0, i, 0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
						activateMesh(l_mesh);
						drawMesh(l_mesh);
					}
				}
			}
		}
	}

	// draw environment pre-filter texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program);
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_p,
		captureProjection);

	if (l_visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : l_visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture;
					auto l_environmentPrefilterTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture;
					activateTexture(l_environmentPrefilterTexture, 2);
					unsigned int maxMipLevels = 5;
					for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
					{
						// resize framebuffer according to mip-level size.
						unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
						unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

						glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
						glViewport(0, 0, mipWidth, mipHeight);

						double roughness = (double)mip / (double)(maxMipLevels - 1);
						updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_roughness, roughness);
						for (unsigned int i = 0; i < 6; ++i)
						{
							updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_r, captureViews[i]);
							attachTextureToFramebuffer(l_environmentPrefilterTexture, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent, 0, i, mip);

							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
							activateMesh(l_mesh);
							drawMesh(l_mesh);
						}
					}
				}
			}
		}
	}

	// draw environment BRDF look-up table texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
	glViewport(0, 0, 512, 512);
	glUseProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram.m_program);

	auto l_environmentBRDFLUTTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture;
	attachTextureToFramebuffer(l_environmentBRDFLUTTexture, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent, 0, 0, 0);

	// draw environment map BRDF LUT rectangle
	auto l_mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::QUAD);
	activateMesh(l_mesh);
	drawMesh(l_mesh);
}

void GLRenderingSystem::updateShadowRenderPass()
{
	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// draw each lightComponent's shadowmap
	for (size_t i = 0; i < 4; i++)
	{
		for (auto& l_lightComponent : g_pGameSystem->getLightComponents())
		{
			if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
			{
				glUseProgram(ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program);
				updateUniform(
					ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p,
					l_lightComponent->getProjectionMatrix(i));
				updateUniform(
					ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v,
					g_pGameSystem->getTransformComponent(l_lightComponent->getParentEntity())->m_transform.getInvertGlobalRotMatrix());

				// draw each visibleComponent
				for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
				{
					if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
					{
						updateUniform(
							ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m,
							g_pGameSystem->getTransformComponent(l_visibleComponent->getParentEntity())->m_transform.caclGlobalTransformationMatrix());

						// draw each graphic data of visibleComponent
						for (auto& l_graphicData : l_visibleComponent->getModelMap())
						{
							// draw meshes
							auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
							activateMesh(l_mesh);
							drawMesh(l_mesh);
						}
					}
				}
			}
		}
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void GLRenderingSystem::updateGeometryRenderPass()
{
	//// bind to framebuffer
	//m_geometryPassFrameBuffer->update(true, true);
	//m_geometryPassFrameBuffer->setRenderBufferStorageSize(0);

	//m_geometryPassShaderProgram->update();

	//mat4 p = cameraComponents[0]->m_projectionMatrix;
	//mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

	//updateUniform(m_uni_p, p);
	//updateUniform(m_uni_r, r);
	//updateUniform(m_uni_t, t);

	///////////////////Blinn-Phong
	//// draw each visibleComponent
	//for (auto& l_visibleComponent : visibleComponents)
	//{
	//	if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
	//	{
	//		updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//		// draw each graphic data of visibleComponent
	//		for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//		{
	//			//active and bind textures
	//			// is there any texture?
	//			auto l_textureMap = &l_graphicData.second;
	//			if (l_textureMap != nullptr)
	//			{
	//				// any normal?
	//				auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//				if (l_normalTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_normalTextureID->second)->second;
	//					l_textureData->update(0);
	//				}
	//				// any diffuse?
	//				auto l_diffuseTextureID = l_textureMap->find(textureType::ALBEDO);
	//				if (l_diffuseTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
	//					l_textureData->update(1);
	//				}
	//				// any specular?
	//				auto l_specularTextureID = l_textureMap->find(textureType::METALLIC);
	//				if (l_specularTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_specularTextureID->second)->second;
	//					l_textureData->update(2);
	//				}
	//			}
	//			// draw meshes
	//			meshMap.find(l_graphicData.first)->second->update();
	//		}
	//	}
	//}
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_CLAMP);

	/////////////////Cook-Torrance
	//if (cameraComponents.size() > 0)
	//{
	//	mat4 p = cameraComponents[0]->m_projectionMatrix;
	//	mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//	mat4 t = cameraComponents[0]->getInvertTranslationMatrix();
	//	updateUniform(m_uni_p, p);
	//	updateUniform(m_uni_r, r);
	//	updateUniform(m_uni_t, t);
	//}

	//if (lightComponents.size() > 0)
	//{
	//	for (auto& l_lightComponent : lightComponents)
	//	{
	//		// update light space transformation matrices
	//		if (l_lightComponent->getLightType() == lightType::DIRECTIONAL)
	//		{
	//			updateUniform(m_uni_p_light, l_lightComponent->getProjectionMatrix(0));
	//			updateUniform(m_uni_v_light, l_lightComponent->getViewMatrix());

	//			// draw each visibleComponent
	//			for (auto& l_visibleComponent : visibleComponents)
	//			{
	//				if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

	//					updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//					// draw each graphic data of visibleComponent
	//					for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//					{
	//						//active and bind textures
	//						// is there any texture?
	//						auto l_textureMap = &l_graphicData.second;
	//						if (l_textureMap != nullptr)
	//						{
	//							// any normal?
	//							auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//							if (l_normalTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_normalTextureID->second)->second;
	//								l_textureData->update(0);
	//							}
	//							// any albedo?
	//							auto l_albedoTextureID = l_textureMap->find(textureType::ALBEDO);
	//							if (l_albedoTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_albedoTextureID->second)->second;
	//								l_textureData->update(1);
	//							}
	//							// any metallic?
	//							auto l_metallicTextureID = l_textureMap->find(textureType::METALLIC);
	//							if (l_metallicTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_metallicTextureID->second)->second;
	//								l_textureData->update(2);
	//							}
	//							// any roughness?
	//							auto l_roughnessTextureID = l_textureMap->find(textureType::ROUGHNESS);
	//							if (l_roughnessTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_roughnessTextureID->second)->second;
	//								l_textureData->update(3);
	//							}
	//							// any ao?
	//							auto l_aoTextureID = l_textureMap->find(textureType::AMBIENT_OCCLUSION);
	//							if (l_aoTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_aoTextureID->second)->second;
	//								l_textureData->update(4);
	//							}
	//						}
	//						updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
	//						updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
	//						updateUniform(m_uni_MRA, l_visibleComponent->m_MRA.x, l_visibleComponent->m_MRA.y, l_visibleComponent->m_MRA.z);
	//						// draw meshes
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//				else if (l_visibleComponent->m_visiblilityType == visiblilityType::EMISSIVE)
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

	//					updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//					// draw each graphic data of visibleComponent
	//					for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//					{
	//						updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
	//						updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
	//						// draw meshes
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//				else
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
	//				}
	//			}
	//		}
	//	}
	//}

	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_CLAMP);
	//glDisable(GL_STENCIL_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLRenderingSystem::updateLightRenderPass()
{
}

void GLRenderingSystem::updateFinalRenderPass()
{
}

void GLRenderingSystem::shutdown()
{
}

const objectStatus & GLRenderingSystem::getStatus() const
{
	return m_objectStatus;
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

void GLRenderingSystem::attachTextureToFramebuffer(const GLTextureDataComponent * GLTextureDataComponent, const GLFrameBufferComponent * GLFrameBufferComponent, int colorAttachmentIndex, int textureIndex, int mipLevel)
{
	if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_textureID);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTextureDataComponent->m_textureID, mipLevel);
	}
	else if (GLTextureDataComponent->m_textureType == textureType::SHADOWMAP)
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_textureID);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTextureDataComponent->m_textureID, mipLevel);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_textureID);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTextureDataComponent->m_textureID, mipLevel);
	}

}

void GLRenderingSystem::activateShaderProgram(const GLShaderProgramComponent * GLShaderProgramComponent)
{
	glUseProgram(GLShaderProgramComponent->m_program);
}

void GLRenderingSystem::activateMesh(const MeshDataComponent * GLTextureDataComponent)
{
	glBindVertexArray(GLTextureDataComponent->m_VAO);
}

void GLRenderingSystem::drawMesh(const MeshDataComponent * GLTextureDataComponent)
{
	glDrawElements(GL_TRIANGLES + (int)GLTextureDataComponent->m_meshDrawMethod, GLTextureDataComponent->m_indices.size(), GL_UNSIGNED_INT, 0);
}

void GLRenderingSystem::activateTexture(const TextureDataComponent * GLTextureDataComponent, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_textureID);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_textureID);
	}

}
