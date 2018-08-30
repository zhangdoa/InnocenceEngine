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
	initializeGeometryRenderPass();
	initializeLightRenderPass();
	initializeFinalRenderPass();
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
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureType = textureType::ENVIRONMENT_CAPTURE;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureWrapMethod = textureWrapMethod::REPEAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureWidth = 2048;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureHeight = 2048;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture.m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	initializeTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture);

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureType = textureType::ENVIRONMENT_CONVOLUTION;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureMinFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureWrapMethod = textureWrapMethod::REPEAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureWidth = 128;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureHeight = 128;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture.m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	initializeTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture);

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureType = textureType::ENVIRONMENT_PREFILTER;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureWrapMethod = textureWrapMethod::REPEAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureWidth = 128;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureHeight = 128;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture.m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	initializeTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture);

	////
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RG16F;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_texturePixelDataFormat = texturePixelDataFormat::RG;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureMinFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureWidth = 512;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureHeight = 512;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture.m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	initializeTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture);

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
		"GL3.3//environmentCapturePassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentCapturePassPBSFragment.sf");

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
		"GL3.3//environmentConvolutionPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentConvolutionPassPBSFragment.sf");
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_capturedCubeMap = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassProgram.m_program,
		"uni_capturedCubeMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_capturedCubeMap,
		0);
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
		"GL3.3//environmentPreFilterPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentPreFilterPassPBSFragment.sf");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_capturedCubeMap = getUniformLocation(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassProgram.m_program,
		"uni_capturedCubeMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_capturedCubeMap,
		0);
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
		"GL3.3//environmentBRDFLUTPassPBSVertex.sf");
	initializeShader(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassProgram.m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentBRDFLUTPassPBSFragment.sf");
}

void GLRenderingSystem::initializeShadowRenderPass()
{
	for (size_t i = 0; i < 4; i++)
	{
		ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector.emplace_back();
	}
	for (size_t i = 0; i < ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector.size(); i++)
	{
		auto l_frameBuffer = &ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[i].first;
		auto l_texture = &ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[i].second;
		// generate and bind framebuffer
		glGenFramebuffers(1, &l_frameBuffer->m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, l_frameBuffer->m_FBO);

		// generate and bind renderbuffer
		glGenRenderbuffers(1, &l_frameBuffer->m_RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_frameBuffer->m_RBO);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_frameBuffer->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

		// generate and bind texture
		l_texture->m_textureType = textureType::SHADOWMAP;
		l_texture->m_textureColorComponentsFormat = textureColorComponentsFormat::DEPTH_COMPONENT;
		l_texture->m_texturePixelDataFormat = texturePixelDataFormat::DEPTH_COMPONENT;
		l_texture->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
		l_texture->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
		l_texture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_BORDER;
		l_texture->m_textureWidth = 2048;
		l_texture->m_textureHeight = 2048;
		l_texture->m_texturePixelDataType = texturePixelDataType::FLOAT;
		l_texture->m_textureData = { nullptr };
		initializeTexture(l_texture);

		attachTextureToFramebuffer(
			l_texture,
			l_frameBuffer,
			0, 0, 0
		);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		// finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::stringstream ss;
			ss << i;
			g_pLogSystem->printLog("GLFrameBuffer: ShadowRenderPass level " + ss.str() +" Framebuffer is not completed!");
		}
	}


	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program = glCreateProgram();
	initializeShader(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//shadowPassVertex.sf");
	initializeShader(
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//shadowPassFragment.sf");

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

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	for (size_t i = 0; i < 8; i++)
	{
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures.emplace_back();
	}
	for (size_t i = 0; i < GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures.size(); i++)
	{
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureType = textureType::RENDER_BUFFER_SAMPLER;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureMinFilterMethod = textureFilterMethod::NEAREST;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureMagFilterMethod = textureFilterMethod::NEAREST;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_texturePixelDataType = texturePixelDataType::FLOAT;
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i].m_textureData = { nullptr };
		initializeTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i]);

		attachTextureToFramebuffer(
			&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[i],
			&GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent,
			(int)i, 0, 0
		);
	}

	std::vector<unsigned int> l_colorAttachments;
	for (unsigned int i = 0; i < 8; ++i)
	{
		l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: GeometryRenderPass Framebuffer is not completed!");
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
		"GL3.3//geometryPassCookTorranceVertex.sf");
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//geometryPassCookTorranceFragment.sf");
#elif BlinnPhong
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//geometryPassBlinnPhongVertex.sf");
	initializeShader(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//geometryPassBlinnPhongFragment.sf");
#endif
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p_camera");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_r_camera");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_t_camera");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_m");
#ifdef CookTorrance
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_previous = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_r_camera_previous");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_previous = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_t_camera_previous");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0 = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p_light_0");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1 = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p_light_1");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2 = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p_light_2");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3 = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_p_light_3");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_v_light");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_normalTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture,
		0);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_albedoTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture,
		1);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_metallicTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture,
		2);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_roughnessTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture,
		3);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_aoTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture,
		4);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useTexture = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_useTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_albedo");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA = getUniformLocation(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program,
		"uni_MRA");
#elif BlinnPhong
	// @TODO: texture uniforms
#endif
}

void GLRenderingSystem::initializeLightRenderPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_textureData = { nullptr };
	initializeTexture(&LightRenderPassSingletonComponent::getInstance().m_lightPassTexture);

	attachTextureToFramebuffer(
		&LightRenderPassSingletonComponent::getInstance().m_lightPassTexture,
		&LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: LightRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program = glCreateProgram();
#ifdef CookTorrance
	initializeShader(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//lightPassCookTorranceVertex.sf");
	initializeShader(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//lightPassCookTorranceFragment.sf");
#elif BlinnPhong
	initializeShader(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//lightPassBlinnPhongVertex.sf");
	initializeShader(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//lightPassBlinnPhongFragment.sf");
#endif
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT0");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0,
		0);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT1");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1,
		1);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT2");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2,
		2);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT4");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4,
		3);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT5");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5,
		4);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT6");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6,
		5);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_geometryPassRT7");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7,
		6);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_shadowMap_0");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0,
		7);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_shadowMap_1");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1,
		8);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_shadowMap_2");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2,
		9);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3 = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_shadowMap_3");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3,
		10);
	LightRenderPassSingletonComponent::getInstance().m_uni_irradianceMap = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_irradianceMap");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_irradianceMap,
		11);
	LightRenderPassSingletonComponent::getInstance().m_uni_preFiltedMap = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_preFiltedMap");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_preFiltedMap,
		12);
	LightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_brdfLUT");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT,
		13);
	LightRenderPassSingletonComponent::getInstance().m_uni_viewPos = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_viewPos");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_position = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_dirLight.position");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_dirLight.direction");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color = getUniformLocation(
		LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program,
		"uni_dirLight.color");
	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < g_pGameSystem->getLightComponents().size(); i++)
	{
		if (g_pGameSystem->getLightComponents()[i]->m_lightType == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
		}
		if (g_pGameSystem->getLightComponents()[i]->m_lightType == lightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position.emplace_back(
				getUniformLocation(LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program, "uni_pointLights[" + ss.str() + "].position")
			);
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius.emplace_back(
				getUniformLocation(LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program, "uni_pointLights[" + ss.str() + "].radius")
			);
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color.emplace_back(
				getUniformLocation(LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program, "uni_pointLights[" + ss.str() + "].color")
			);
		}
	}
}

void GLRenderingSystem::initializeFinalRenderPass()
{
	initializeSkyPass();
	initializeBloomExtractPass();
	initializeBloomBlurPass();
	initializeMotionBlurPass();
	initializeBillboardPass();
	initializeDebuggerPass();
	initializeFinalBlendPass();
}

void GLRenderingSystem::initializeSkyPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: ShadowRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//skyPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//skyPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_skybox = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program,
		"uni_skybox");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_skybox,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_p = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_r = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program,
		"uni_r");
}

void GLRenderingSystem::initializeBloomExtractPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: BloomExtractRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//bloomExtractPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//bloomExtractPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_lightPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program,
		"uni_lightPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_lightPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_isEmissive = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program,
		"uni_isEmissive");
}

void GLRenderingSystem::initializeBloomBlurPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_pingPassColorAttachments;
	l_pingPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pingPassColorAttachments.size(), &l_pingPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: BloomBlurPingRenderPass Framebuffer is not completed!");
	}

	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_pongPassColorAttachments;
	l_pongPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pongPassColorAttachments.size(), &l_pongPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: BloomBlurPongRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//bloomBlurPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//bloomBlurPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_bloomExtractPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program,
		"uni_bloomExtractPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_bloomExtractPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program,
		"uni_horizontal");
}

void GLRenderingSystem::initializeMotionBlurPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_pongPassColorAttachments;
	l_pongPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pongPassColorAttachments.size(), &l_pongPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: MotionBlurRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//motionBlurPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//motionBlurPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_motionVectorTexture = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program,
		"uni_motionVectorTexture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_motionVectorTexture,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_lightPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program,
		"uni_lightPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_lightPassRT0,
		1);
}

void GLRenderingSystem::initializeBillboardPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: BillboardRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//billboardPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//billboardPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_texture = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_texture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_p = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_r = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_r");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_t = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_t");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_pos = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_pos");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_albedo = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_albedo");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program,
		"uni_size");
}

void GLRenderingSystem::initializeDebuggerPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: DebuggerRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//debuggerPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//debuggerPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		"uni_normalTexture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_p = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_r = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		"uni_r");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_t = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		"uni_t");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program,
		"uni_m");
}

void GLRenderingSystem::initializeFinalBlendPass()
{
	// generate and bind framebuffer
	glGenFramebuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// generate and bind texture
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureWidth = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureHeight = (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_texturePixelDataType = texturePixelDataType::FLOAT;
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_textureData = { nullptr };
	initializeTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture);

	attachTextureToFramebuffer(
		&GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture,
		&GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent,
		0, 0, 0
	);

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pLogSystem->printLog("GLFrameBuffer: FinalBlendRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// shader programs and shaders
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program = glCreateProgram();
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//finalBlendPassVertex.sf");
	initializeShader(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//finalBlendPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		"uni_motionBlurPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_skyPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		"uni_skyPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_skyPassRT0,
		1);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		"uni_bloomPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0,
		2);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		"uni_billboardPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0,
		3);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0 = getUniformLocation(
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program,
		"uni_debuggerPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0,
		4);
}

void GLRenderingSystem::initializeShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath)
{
	shaderID = glCreateShader(shaderType);

	if (shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
	}

	auto l_shaderCodeContent = g_pAssetSystem->loadShader(shaderFilePath);
	const char* l_sourcePointer = l_shaderCodeContent.c_str();

	if(l_sourcePointer == nullptr)
	{
        g_pLogSystem->printLog("Error: Shader loading failed!");
	}

	glShaderSource(shaderID, 1, &l_sourcePointer, NULL);
    glCompileShader(shaderID);

	GLint l_compileResult = GL_FALSE;
	GLint l_infoLogLength = 0;
	GLint l_shaderFileLength = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);

	if (!l_compileResult)
	{
		g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " compile failed!");
        glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
        g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
            std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
            glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
            g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
            return;
        }
	}

    g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been compiled.");

	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " is linking ...");

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);
	if (!l_compileResult)
	{
		g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " link failed!");
        glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
        g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
            std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
            glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
            g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
        }
	}

	g_pLogSystem->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been linked.");
}

void GLRenderingSystem::initializeDefaultGraphicPrimtives()
{
	{	auto l_Mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::LINE);
	initializeMesh(l_Mesh);
	RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_Mesh->m_meshID, l_Mesh); }
	{	auto l_Mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::QUAD);
	initializeMesh(l_Mesh);
	RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_Mesh->m_meshID, l_Mesh); }
	{	auto l_Mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::CUBE);
	initializeMesh(l_Mesh);
	RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_Mesh->m_meshID, l_Mesh); }
	{	auto l_Mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::SPHERE);
	initializeMesh(l_Mesh);
	RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_Mesh->m_meshID, l_Mesh); }


	{	auto l_Texture = g_pAssetSystem->getDefaultTexture(textureType::NORMAL);
	initializeTexture(l_Texture);
	RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(l_Texture->m_textureID, l_Texture); }
	{	auto l_Texture = g_pAssetSystem->getDefaultTexture(textureType::ALBEDO);
	initializeTexture(l_Texture);
	RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(l_Texture->m_textureID, l_Texture); }
	{	auto l_Texture = g_pAssetSystem->getDefaultTexture(textureType::METALLIC);
	initializeTexture(l_Texture);
	RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(l_Texture->m_textureID, l_Texture); }
	{	auto l_Texture = g_pAssetSystem->getDefaultTexture(textureType::ROUGHNESS);
	initializeTexture(l_Texture);
	RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(l_Texture->m_textureID, l_Texture); }
	{	auto l_Texture = g_pAssetSystem->getDefaultTexture(textureType::AMBIENT_OCCLUSION);
	initializeTexture(l_Texture);
	RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(l_Texture->m_textureID, l_Texture); }
}

void GLRenderingSystem::initializeGraphicPrimtivesOfComponents()
{
	for (auto& l_cameraComponent : g_pGameSystem->getCameraComponents())
	{
		if (l_cameraComponent->m_drawAABB)
		{
			auto l_meshID = g_pAssetSystem->addMesh(l_cameraComponent->m_AABB.m_vertices, l_cameraComponent->m_AABB.m_indices);
			auto l_Mesh = g_pAssetSystem->getMesh(l_meshID);
			initializeMesh(l_Mesh);
			RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_cameraComponent->m_AABBMeshID, l_Mesh);
			l_cameraComponent->m_AABBMeshID = l_meshID;
		}
		if (l_cameraComponent->m_drawFrustum)
		{
			auto l_meshID = g_pAssetSystem->addMesh(l_cameraComponent->m_frustumVertices, l_cameraComponent->m_frustumIndices);
			auto l_Mesh = g_pAssetSystem->getMesh(l_meshID);
			initializeMesh(l_Mesh);
			RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_cameraComponent->m_FrustumMeshID, l_Mesh);
			l_cameraComponent->m_FrustumMeshID = l_meshID;
		}
	}
	for (auto& l_lightComponent : g_pGameSystem->getLightComponents())
	{
		if (l_lightComponent->m_drawAABB)
		{
			for (size_t i = 0; i < l_lightComponent->m_AABBMeshIDs.size(); i++)
			{
				auto l_meshID = g_pAssetSystem->addMesh(l_lightComponent->m_AABBs[i].m_vertices, l_lightComponent->m_AABBs[i].m_indices);
				auto l_Mesh = g_pAssetSystem->getMesh(l_meshID);
				initializeMesh(l_Mesh);
				RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_lightComponent->m_AABBMeshIDs[i], l_Mesh);
				l_lightComponent->m_AABBMeshIDs.emplace_back(l_meshID);
			}
		}
	}
	for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
	{
		for (auto& l_graphicData : l_visibleComponent->m_modelMap)
		{
			if (RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.find(l_graphicData.first) == RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.end())
			{
				auto l_Mesh = g_pAssetSystem->getMesh(l_graphicData.first);
				initializeMesh(l_Mesh);
				RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_graphicData.first, l_Mesh);
			}
			std::for_each(l_graphicData.second.begin(), l_graphicData.second.end(), [&](texturePair val) {
				if (RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.find(val.second) == RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.end())
				{
					auto l_Texture = g_pAssetSystem->getTexture(val.second);
					initializeTexture(l_Texture);
					RenderingSystemSingletonComponent::getInstance().m_initializedTextureMap.emplace(val.second, l_Texture);
				}
			});
		}
		// @TODO: AABB's mesh shouldn't be the first class citizen
		if (l_visibleComponent->m_drawAABB)
		{
			auto l_meshID = g_pAssetSystem->addMesh(l_visibleComponent->m_AABB.m_vertices, l_visibleComponent->m_AABB.m_indices);
			auto l_Mesh = g_pAssetSystem->getMesh(l_meshID);
			initializeMesh(l_Mesh);
			RenderingSystemSingletonComponent::getInstance().m_initializedMeshMap.emplace(l_visibleComponent->m_AABBMeshID, l_Mesh);
			l_visibleComponent->m_AABBMeshID = l_meshID;
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
	if (RenderingSystemSingletonComponent::getInstance().m_shouldUpdateEnvironmentMap)
	{
		updateEnvironmentRenderPass();
		RenderingSystemSingletonComponent::getInstance().m_shouldUpdateEnvironmentMap = false;
	}
	updateShadowRenderPass();
	updateGeometryRenderPass();
	updateLightRenderPass();
	updateFinalRenderPass();
}

void GLRenderingSystem::updateEnvironmentRenderPass()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);

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
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
				{
					// activate equiretangular texture and remap equiretangular texture to cubemap
					auto l_equiretangularTexture = g_pAssetSystem->getTexture(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
					activateTexture(l_equiretangularTexture, 0);
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(
							EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r,
							captureViews[i]);
						attachTextureToFramebuffer(&EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture, &EnvironmentRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent, 0, i, 0);
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
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
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
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
				{
					auto l_environmentCaptureTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture;
					auto l_environmentPrefilterTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture;
					activateTexture(l_environmentCaptureTexture, 2);
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	// draw each lightComponent's shadowmap
	for (size_t i = 0; i < 4; i++)
	{
		// bind to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[i].first.m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[i].first.m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
		glViewport(0, 0, 2048, 2048);

		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (auto& l_lightComponent : g_pGameSystem->getLightComponents())
		{
			if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
			{
				glUseProgram(ShadowRenderPassSingletonComponent::getInstance().m_shadowPassProgram.m_program);
				updateUniform(
					ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p,
					g_pGameSystem->getProjectionMatrix(l_lightComponent, (unsigned int)i));					
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
						for (auto& l_graphicData : l_visibleComponent->m_modelMap)
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

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void GLRenderingSystem::updateGeometryRenderPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);

	glUseProgram(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassProgram.m_program);

	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		mat4 p = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
		mat4 r = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();
		mat4 t = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalTranslationMatrix();
		mat4 r_prev = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getPreviousInvertGlobalRotMatrix();
		mat4 t_prev = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getPreviousInvertGlobalTranslationMatrix();

		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera,
			p);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera,
			r);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera,
			t);

		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_previous,
			r_prev);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_previous,
			t_prev);

#ifdef CookTorrance
		//Cook-Torrance
		if (g_pGameSystem->getLightComponents().size() > 0)
		{
			for (auto& l_lightComponent : g_pGameSystem->getLightComponents())
			{
				// update light space transformation matrices
				if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
				{
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0,
						g_pGameSystem->getProjectionMatrix(l_lightComponent, 0));
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1,
						g_pGameSystem->getProjectionMatrix(l_lightComponent, 1));
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2,
						g_pGameSystem->getProjectionMatrix(l_lightComponent, 2));
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3,
						g_pGameSystem->getProjectionMatrix(l_lightComponent, 3));
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light,
						g_pGameSystem->getTransformComponent(l_lightComponent->getParentEntity())->m_transform.getInvertGlobalRotMatrix());

					// draw each visibleComponent
					for (auto& l_visibleComponent : RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents)
					{
						if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
						{
							glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								g_pGameSystem->getTransformComponent(l_visibleComponent->getParentEntity())->m_transform.caclGlobalTransformationMatrix());

							// draw each graphic data of visibleComponent
							for (auto& l_graphicData : l_visibleComponent->m_modelMap)
							{
								//active and bind textures
								// is there any texture?
								auto l_textureMap = &l_graphicData.second;
								if (l_textureMap != nullptr)
								{
									// any normal?
									auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
									if (l_normalTextureID != l_textureMap->end())
									{
										auto l_textureData = g_pAssetSystem->getTexture(l_normalTextureID->second);
										activateTexture(l_textureData, 0);
									}
									// any albedo?
									auto l_albedoTextureID = l_textureMap->find(textureType::ALBEDO);
									if (l_albedoTextureID != l_textureMap->end())
									{
										auto l_textureData = g_pAssetSystem->getTexture(l_albedoTextureID->second);
										activateTexture(l_textureData, 1);
									}
									// any metallic?
									auto l_metallicTextureID = l_textureMap->find(textureType::METALLIC);
									if (l_metallicTextureID != l_textureMap->end())
									{
										auto l_textureData = g_pAssetSystem->getTexture(l_metallicTextureID->second);
										activateTexture(l_textureData, 2);
									}
									// any roughness?
									auto l_roughnessTextureID = l_textureMap->find(textureType::ROUGHNESS);
									if (l_roughnessTextureID != l_textureMap->end())
									{
										auto l_textureData = g_pAssetSystem->getTexture(l_roughnessTextureID->second);
										activateTexture(l_textureData, 3);
									}
									// any ao?
									auto l_aoTextureID = l_textureMap->find(textureType::AMBIENT_OCCLUSION);
									if (l_aoTextureID != l_textureMap->end())
									{
										auto l_textureData = g_pAssetSystem->getTexture(l_aoTextureID->second);
										activateTexture(l_textureData, 4);
									}
								}
								updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useTexture, l_visibleComponent->m_useTexture);
								updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
								updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA, l_visibleComponent->m_MRA.x, l_visibleComponent->m_MRA.y, l_visibleComponent->m_MRA.z);
								// draw meshes
								auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
								activateMesh(l_mesh);
								drawMesh(l_mesh);
							}
						}
						else if (l_visibleComponent->m_visiblilityType == visiblilityType::EMISSIVE)
						{
							glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								g_pGameSystem->getTransformComponent(l_visibleComponent->getParentEntity())->m_transform.caclGlobalTransformationMatrix());

							// draw each graphic data of visibleComponent
							for (auto& l_graphicData : l_visibleComponent->m_modelMap)
							{
								updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useTexture, l_visibleComponent->m_useTexture);
								updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
								// draw meshes
								auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
								activateMesh(l_mesh);
								drawMesh(l_mesh);
							}
						}
						else
						{
							glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
						}
					}
				}
			}
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_CLAMP);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#elif BlinnPhong
		// draw each visibleComponent
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				updateUniform(
					GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
					g_pGameSystem->getTransformComponent(l_visibleComponent->getParentEntity())->m_transform.caclGlobalTransformationMatrix());

				// draw each graphic data of visibleComponent
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
				{
					//active and bind textures
					// is there any texture?
					auto l_textureMap = &l_graphicData.second;
					if (l_textureMap != nullptr)
					{
						// any normal?
						auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
						if (l_normalTextureID != l_textureMap->end())
						{
							auto l_textureData = g_pAssetSystem->getTexture(l_normalTextureID->second);
							activateTexture(l_textureData, 0);
						}
						// any diffuse?
						auto l_diffuseTextureID = l_textureMap->find(textureType::ALBEDO);
						if (l_diffuseTextureID != l_textureMap->end())
						{
							auto l_textureData = g_pAssetSystem->getTexture(l_diffuseTextureID->second);
							activateTexture(l_textureData, 1);
						}
						// any specular?
						auto l_specularTextureID = l_textureMap->find(textureType::METALLIC);
						if (l_specularTextureID != l_textureMap->end())
						{
							auto l_textureData = g_pAssetSystem->getTexture(l_specularTextureID->second);
							activateTexture(l_textureData, 2);
						}
					}
					// draw meshes
					auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
					activateMesh(l_mesh);
					drawMesh(l_mesh);
				}
			}
	}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_DEPTH_CLAMP);
#endif
}
}

void GLRenderingSystem::updateLightRenderPass()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, LightRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	glUseProgram(LightRenderPassSingletonComponent::getInstance().m_lightPassProgram.m_program);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[0], 0);
	// normal + roughness
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[1], 1);
	// albedo + ambient occlusion
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[2], 2);
	// light space position 0
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[4], 3);
	// light space position 1
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[5], 4);
	// light space position 2
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[6], 5);
	// light space position 3
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[7], 6);
	// shadow map 0
	activateTexture(&ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[0].second, 7);
	// shadow map 1
	activateTexture(&ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[1].second, 8);
	// shadow map 2
	activateTexture(&ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[2].second, 9);
	// shadow map 3
	activateTexture(&ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[3].second, 10);
	// irradiance environment map
	activateTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTexture, 11);
	// pre-filter specular environment map
	activateTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTexture, 12);
	// BRDF look-up table
	activateTexture(&EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTexture, 13);
#endif
	if (g_pGameSystem->getLightComponents().size() > 0)
	{
		int l_pointLightIndexOffset = 0;
		for (auto i = (unsigned int)0; i < g_pGameSystem->getLightComponents().size(); i++)
		{
			auto l_viewPos = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.caclGlobalPos();
			auto l_lightPos = g_pGameSystem->getTransformComponent(g_pGameSystem->getLightComponents()[i]->getParentEntity())->m_transform.caclGlobalPos();
			auto l_dirLightDirection = g_pGameSystem->getTransformComponent(g_pGameSystem->getLightComponents()[i]->getParentEntity())->m_transform.getDirection(direction::BACKWARD);
			auto l_lightColor = g_pGameSystem->getLightComponents()[i]->m_color;
			updateUniform(
				LightRenderPassSingletonComponent::getInstance().m_uni_viewPos,
				l_viewPos.x, l_viewPos.y, l_viewPos.z);
			//updateUniform(m_uni_textureMode, (int)in_shaderDrawPair.second);

			if (g_pGameSystem->getLightComponents()[i]->m_lightType == lightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_position,
					l_lightPos.x, l_lightPos.y, l_lightPos.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction,
					l_dirLightDirection.x, l_dirLightDirection.y, l_dirLightDirection.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color,
					l_lightColor.x, l_lightColor.y, l_lightColor.z);
			}
			else if (g_pGameSystem->getLightComponents()[i]->m_lightType == lightType::POINT)
			{
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position[i + l_pointLightIndexOffset],
					l_lightPos.x, l_lightPos.y, l_lightPos.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius[i + l_pointLightIndexOffset],
					g_pGameSystem->getLightComponents()[i]->m_radius);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color[i + l_pointLightIndexOffset],
					l_lightColor.x, l_lightColor.y, l_lightColor.z);
			}
		}
	}
	// draw light pass rectangle
	auto l_mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::QUAD);
	activateMesh(l_mesh);
	drawMesh(l_mesh);

	glDisable(GL_STENCIL_TEST);
}

void GLRenderingSystem::updateFinalRenderPass()
{
	// sky pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_FRONT);

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_skyPassProgram.m_program);

	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		mat4 p = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
		mat4 r = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();

		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_p,
			p);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_r,
			r);

		auto& l_visibleComponents = g_pGameSystem->getVisibleComponents();
		if (l_visibleComponents.size() > 0)
		{
			for (auto& l_visibleComponent : l_visibleComponents)
			{
				if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
				{
					for (auto& l_graphicData : l_visibleComponent->m_modelMap)
					{
						// use environment pass capture texture as skybox cubemap
						auto l_skyboxTexture = &EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTexture;
						activateTexture(l_skyboxTexture, 0);
						auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
						activateMesh(l_mesh);
						drawMesh(l_mesh);
					}
				}
			}
		}
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// bloom extract pass
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassProgram.m_program);

	// 1. extract bright part from light pass
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_isEmissive,
		false);
	activateTexture(&LightRenderPassSingletonComponent::getInstance().m_lightPassTexture, 0);
	auto l_mesh = g_pAssetSystem->getDefaultMesh(meshShapeType::QUAD);
	activateMesh(l_mesh);
	drawMesh(l_mesh);

	// 2. draw emissive mesh
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLFrameBufferComponent.m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_isEmissive,
		true);
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[2], 0);
	activateMesh(l_mesh);
	drawMesh(l_mesh);

	glDisable(GL_STENCIL_TEST);

	// bloom blur pass
	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassProgram.m_program);

	bool l_isPing = true;
	bool l_isFirstIteration = true;
	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_FBO);
			glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLFrameBufferComponent.m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
			glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);

			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				true);
			if (l_isFirstIteration)
			{
				activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTexture, 0);
				l_isFirstIteration = false;
			}
			else
			{
				activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture, 0);
			}

			activateMesh(l_mesh);
			drawMesh(l_mesh);
			l_isPing = false;
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_FBO);
			glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLFrameBufferComponent.m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
			glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);

			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				false);
			activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTexture, 0);

			activateMesh(l_mesh);
			drawMesh(l_mesh);
			l_isPing = true;
		}
	}

	// motion blur pass
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassProgram.m_program);
	activateTexture(&GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[3], 0);
	activateTexture(&LightRenderPassSingletonComponent::getInstance().m_lightPassTexture, 1);

	activateMesh(l_mesh);
	drawMesh(l_mesh);

	// billboard pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	// copy depth buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLFrameBufferComponent.m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassProgram.m_program);

	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		mat4 p = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
		mat4 r = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();
		mat4 t = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalTranslationMatrix();

		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_p,
			p);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_r,
			r);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_t,
			t);
	}
	if(g_pGameSystem->getVisibleComponents().size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::BILLBOARD)
			{
				auto l_GlobalPos = g_pGameSystem->getTransformComponent(l_visibleComponent->getParentEntity())->m_transform.caclGlobalPos();
				auto l_GlobalCameraPos = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.caclGlobalPos();
				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_pos,
					l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);
				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_albedo,
					l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
				auto l_distanceToCamera = (l_GlobalCameraPos - l_GlobalPos).length();
				if (l_distanceToCamera > 1.0)
				{
					updateUniform(
						GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size,
						(1.0 / l_distanceToCamera) * (9.0 / 16.0), (1.0 / l_distanceToCamera));
				}
				else
				{
					updateUniform(
						GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size,
						(9.0 / 16.0), 1.0);
				}
				// draw each graphic data of visibleComponent
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
				{
					//active and bind textures
					// is there any texture?
					auto l_textureMap = &l_graphicData.second;
					if (l_textureMap != nullptr)
					{
						// any normal?
						auto l_normalTextureID = l_textureMap->find(textureType::ALBEDO);
						if (l_normalTextureID != l_textureMap->end())
						{
							auto l_textureData = g_pAssetSystem->getTexture(l_normalTextureID->second);
							activateTexture(l_textureData, 0);
						}
						auto l_mesh = g_pAssetSystem->getMesh(l_graphicData.first);
						activateMesh(l_mesh);
						drawMesh(l_mesh);
					}
				}
			}
		}
	}

	glDisable(GL_DEPTH_TEST);

	// debugger pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	// copy depth buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_GLFrameBufferComponent.m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLFrameBufferComponent.m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, 0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassProgram.m_program);

	//if (g_pGameSystem->getCameraComponents().size() > 0)
	//{
	//	mat4 p = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
	//	mat4 r = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();
	//	mat4 t = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalTranslationMatrix();

	//	updateUniform(
	//		FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_p,
	//		p);
	//	updateUniform(
	//		FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_r,
	//		r);
	//	updateUniform(
	//		FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_t,
	//		t);

	//	for (auto& l_cameraComponent : g_pGameSystem->getCameraComponents())
	//	{
	//		// draw frustum for cameraComponent
	//		if (l_cameraComponent->m_drawFrustum)
	//		{
	//			auto l_cameraLocalMat = mat4();
	//			l_cameraLocalMat.initializeToIdentityMatrix();
	//			updateUniform(
	//				FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
	//				l_cameraLocalMat);
	//			auto l_mesh = g_pAssetSystem->getMesh(l_cameraComponent->m_FrustumMeshID);
	//			activateMesh(l_mesh);
	//			drawMesh(l_mesh);
	//		}
	//		// draw AABB of frustum for cameraComponent
	//		if (l_cameraComponent->m_drawAABB)
	//		{
	//			auto l_cameraLocalMat = mat4();
	//			l_cameraLocalMat.initializeToIdentityMatrix();
	//			updateUniform(
	//				FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
	//				l_cameraLocalMat);
	//			auto l_mesh = g_pAssetSystem->getMesh(l_cameraComponent->m_AABBMeshID);
	//			activateMesh(l_mesh);
	//			drawMesh(l_mesh);
	//		}
	//	}
	//}
	//if (g_pGameSystem->getLightComponents().size() > 0)
	//{
	//	// draw AABB for lightComponent
	//	for (auto& l_lightComponent : g_pGameSystem->getLightComponents())
	//	{
	//		if (l_lightComponent->m_drawAABB)
	//		{
	//			auto l_lightLocalMat = g_pGameSystem->getTransformComponent(l_lightComponent->getParentEntity())->m_transform.caclGlobalRotMatrix();
	//			updateUniform(
	//				FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
	//				l_lightLocalMat);
	//			for (auto l_AABBMeshID : l_lightComponent->m_AABBMeshIDs)
	//			{
	//				auto l_mesh = g_pAssetSystem->getMesh(l_AABBMeshID);
	//				activateMesh(l_mesh);
	//				drawMesh(l_mesh);
	//			}
	//		}
	//	}
	//}
	//if (g_pGameSystem->getVisibleComponents().size() > 0)
	//{
	//	// draw AABB for visibleComponent
	//	for (auto& l_visibleComponent : g_pGameSystem->getVisibleComponents())
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH && l_visibleComponent->m_drawAABB)
	//		{
	//			auto l_m = mat4();
	//			l_m.initializeToIdentityMatrix();
	//			updateUniform(
	//				FinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
	//				l_m);
	//			// draw each graphic data of visibleComponent
	//			for (auto& l_graphicData : l_visibleComponent->m_modelMap)
	//			{
	//				//active and bind textures
	//				// is there any texture?
	//				auto l_textureMap = &l_graphicData.second;
	//				if (l_textureMap != nullptr)
	//				{
	//					// any normal?
	//					auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//					if (l_normalTextureID != l_textureMap->end())
	//					{
	//						auto l_textureData = g_pAssetSystem->getTexture(l_normalTextureID->second);
	//						activateTexture(l_textureData, 0);
	//					}
	//				}
	//				// draw meshes
	//				auto l_mesh = g_pAssetSystem->getMesh(l_visibleComponent->m_AABBMeshID);
	//				activateMesh(l_mesh);
	//				drawMesh(l_mesh);
	//			}
	//		}
	//	}
	//}

	glDisable(GL_DEPTH_TEST);

	// final blend pass
	glBindFramebuffer(GL_FRAMEBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLFrameBufferComponent.m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (GLsizei)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassProgram.m_program);

	// motion blur pass rendering target
	activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTexture, 0);
	// sky pass rendering target
	activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTexture, 1);
	// bloom pass rendering target
	activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTexture, 2);
	// billboard pass rendering target
	activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTexture, 3);
	// debugger pass rendering target
	activateTexture(&GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTexture, 4);

	//// draw final pass rectangle
	activateMesh(l_mesh);
	drawMesh(l_mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
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
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("GLRenderingSystem: innoShader: Error: Uniform lost: " + uniformName);
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
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTextureDataComponent->m_TAO, mipLevel);
	}
	else if (GLTextureDataComponent->m_textureType == textureType::SHADOWMAP)
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTextureDataComponent->m_TAO, mipLevel);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTextureDataComponent->m_TAO, mipLevel);
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
	glDrawElements(GL_TRIANGLES + (int)GLTextureDataComponent->m_meshDrawMethod, (GLsizei)GLTextureDataComponent->m_indices.size(), GL_UNSIGNED_INT, 0);
}

void GLRenderingSystem::activateTexture(const TextureDataComponent * GLTextureDataComponent, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	if (GLTextureDataComponent->m_textureType == textureType::CUBEMAP || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CAPTURE || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || GLTextureDataComponent->m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTextureDataComponent->m_TAO);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, GLTextureDataComponent->m_TAO);
	}
}
