#include "GLRenderingSystemUtilities.h"
#include "GLEnvironmentRenderingPassUtilities.h"
#include "../component/GLEnvironmentRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "../component/GLFinalRenderPassComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeBRDFLUTPass();
	void initializeEnvironmentCapturePass();

	void updateBRDFLUTPass();
	void updateEnvironmentCapturePass();

	TextureDataDesc BRDFLUTSplitSummingTextureDesc = TextureDataDesc();
	TextureDataDesc BRDFLUTAverangeMSTextureDesc = TextureDataDesc();
	TextureDataDesc EnvCapPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvConvPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvPreFilterPassTextureDesc = TextureDataDesc();
}

void GLRenderingSystemNS::initializeEnvironmentPass()
{
	BRDFLUTSplitSummingTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	BRDFLUTSplitSummingTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	BRDFLUTSplitSummingTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	BRDFLUTSplitSummingTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTSplitSummingTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTSplitSummingTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	BRDFLUTSplitSummingTextureDesc.textureWidth = 512;
	BRDFLUTSplitSummingTextureDesc.textureHeight = 512;
	BRDFLUTSplitSummingTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	BRDFLUTAverangeMSTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	BRDFLUTAverangeMSTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RG16F;
	BRDFLUTAverangeMSTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RG;
	BRDFLUTAverangeMSTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTAverangeMSTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTAverangeMSTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	BRDFLUTAverangeMSTextureDesc.textureWidth = 512;
	BRDFLUTAverangeMSTextureDesc.textureHeight = 512;
	BRDFLUTAverangeMSTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvCapPassTextureDesc.textureUsageType = TextureUsageType::CUBEMAP;
	EnvCapPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvCapPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvCapPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	EnvCapPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvCapPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvCapPassTextureDesc.textureWidth = 2048;
	EnvCapPassTextureDesc.textureHeight = 2048;
	EnvCapPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvConvPassTextureDesc.textureUsageType = TextureUsageType::CUBEMAP;
	EnvConvPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvConvPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvConvPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	EnvConvPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvConvPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvConvPassTextureDesc.textureWidth = 128;
	EnvConvPassTextureDesc.textureHeight = 128;
	EnvConvPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvPreFilterPassTextureDesc.textureUsageType = TextureUsageType::CUBEMAP;
	EnvPreFilterPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvPreFilterPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvPreFilterPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	EnvPreFilterPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvPreFilterPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvPreFilterPassTextureDesc.textureWidth = 128;
	EnvPreFilterPassTextureDesc.textureHeight = 128;
	EnvPreFilterPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	initializeBRDFLUTPass();
	initializeEnvironmentCapturePass();
}

void GLRenderingSystemNS::initializeBRDFLUTPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);

	GLEnvironmentRenderPassComponent::get().m_BRDFLUTPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = BRDFLUTSplitSummingTextureDesc;
	l_TDC->m_textureData = { nullptr };

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassTDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC = l_GLTDC;

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = BRDFLUTAverangeMSTextureDesc;
	l_TDC->m_textureData = { nullptr };

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC = l_GLTDC;

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: BRDFLUTRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	////
	m_ShaderFilePaths.m_VSPath = "GL4.0//BRDFLUTPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//BRDFLUTPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassSPC = rhs;

	////
	m_ShaderFilePaths.m_VSPath = "GL4.0//BRDFLUTMSPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//BRDFLUTMSPassFragment.sf";

	rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT = getUniformLocation(
		rhs->m_program,
		"uni_brdfLUT");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT,
		0);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC = rhs;
}

void GLRenderingSystemNS::initializeEnvironmentCapturePass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

	GLEnvironmentRenderPassComponent::get().m_capturePassFBC = l_FBC;

	// generate and bind texture
	// Capture pass
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvCapPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	GLEnvironmentRenderPassComponent::get().m_capturePassTDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_capturePassGLTDC = l_GLTDC;

	// Convolution pass
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvConvPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	GLEnvironmentRenderPassComponent::get().m_convPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_convPassGLTDC = l_GLTDC;

	// Pre-filter pass
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvPreFilterPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	GLEnvironmentRenderPassComponent::get().m_preFilterPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_preFilterPassGLTDC = l_GLTDC;

	// capture pass shader
	auto rhs = addGLShaderProgramComponent(0);
	initializeGLShaderProgramComponent(rhs, GLEnvironmentRenderPassComponent::get().m_capturePassShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_albedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_albedoTexture");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_capturePass_uni_albedoTexture,
		0);
	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_v = getUniformLocation(
		rhs->m_program,
		"uni_v");
	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");
	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_useAlbedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_useAlbedoTexture");
	GLEnvironmentRenderPassComponent::get().m_capturePass_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");

	GLEnvironmentRenderPassComponent::get().m_capturePassSPC = rhs;

	// conv pass shader
	rhs = addGLShaderProgramComponent(0);
	initializeGLShaderProgramComponent(rhs, GLEnvironmentRenderPassComponent::get().m_convPassShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_convPass_uni_capturedCubeMap = getUniformLocation(
		rhs->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_convPass_uni_capturedCubeMap,
		0);
	GLEnvironmentRenderPassComponent::get().m_convPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLEnvironmentRenderPassComponent::get().m_convPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");

	GLEnvironmentRenderPassComponent::get().m_convPassSPC = rhs;

	// pre-filter pass shader
	rhs = addGLShaderProgramComponent(0);
	initializeGLShaderProgramComponent(rhs, GLEnvironmentRenderPassComponent::get().m_preFilterPassShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_capturedCubeMap = getUniformLocation(
		rhs->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_capturedCubeMap,
		0);
	GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_roughness = getUniformLocation(
		rhs->m_program,
		"uni_roughness");

	GLEnvironmentRenderPassComponent::get().m_preFilterPassSPC = rhs;

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: EnvironmentCaptureRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingSystemNS::updateEnvironmentPass()
{
	updateBRDFLUTPass();
	updateEnvironmentCapturePass();
}

void GLRenderingSystemNS::updateBRDFLUTPass()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = GLEnvironmentRenderPassComponent::get().m_BRDFLUTPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
	glViewport(0, 0, 512, 512);

	// draw split-Sum LUT
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassSPC);
	auto l_SSTDC = GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassTDC;
	auto l_SSGLTDC = GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC;
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	attachTextureToFramebuffer(l_SSTDC, l_SSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawMesh(l_MDC);

	// draw averange RsF1 LUT
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC);
	auto l_MSTDC = GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassTDC;
	auto l_MSGLTDC = GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC;
	attachTextureToFramebuffer(l_MSTDC, l_MSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	activateTexture(l_SSGLTDC, 0);
	drawMesh(l_MDC);
}

void GLRenderingSystemNS::updateEnvironmentCapturePass()
{
	// bind to framebuffer
	auto l_FBC = GLEnvironmentRenderPassComponent::get().m_capturePassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
	glViewport(0, 0, 2048, 2048);

	auto l_mainCapture = GameSystemComponent::get().m_EnvironmentCaptureComponents[0];
	auto l_mainCaptureTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCapture->m_parentEntity);
	//auto l_capturePos = l_mainCaptureTransformComponent->m_localTransformVector.m_pos;
	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mat4 l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> l_v =
	{
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(-1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f, -1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f,  1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f, -1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);

	auto l_capturePassTDC = GLEnvironmentRenderPassComponent::get().m_capturePassTDC;
	auto l_capturePassGLTDC = GLEnvironmentRenderPassComponent::get().m_capturePassGLTDC;

	// draw sky
	// @TODO: optimize
	if (RenderingSystemComponent::get().m_drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		activateShaderProgram(GLFinalRenderPassComponent::get().m_skyPassGLSPC);

		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_p,
			l_p);

		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize,
			2048.0f, 2048.0f);

		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos,
			GLRenderingSystemComponent::get().m_CamGlobalPos.x, GLRenderingSystemComponent::get().m_CamGlobalPos.y, GLRenderingSystemComponent::get().m_CamGlobalPos.z);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
			GLRenderingSystemComponent::get().m_sunDir.x, GLRenderingSystemComponent::get().m_sunDir.y, GLRenderingSystemComponent::get().m_sunDir.z);

		for (unsigned int i = 0; i < 6; ++i)
		{
			updateUniform(GLFinalRenderPassComponent::get().m_skyPass_uni_r, l_v[i]);
			attachTextureToFramebuffer(l_capturePassTDC, l_capturePassGLTDC, l_FBC, 0, i, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}

	// draw opaque meshes
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_capturePassSPC);
	updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_p, l_p);

	for (unsigned int i = 0; i < 6; ++i)
	{
		auto l_copy = GLRenderingSystemComponent::get().m_GPassOpaqueRenderDataQueue_copy;
		updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_v, l_v[i]);
		attachTextureToFramebuffer(l_capturePassTDC, l_capturePassGLTDC, l_FBC, 0, i, 0);
		while (l_copy.size() > 0)
		{
			auto l_renderPack = l_copy.front();
			if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
			{
				if (l_renderPack.m_GPassTextureUBOData.useAlbedoTexture)
				{
					activateTexture(l_renderPack.m_basicAlbedoGLTDC, 0);
				}
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_m, l_renderPack.m_GPassMeshUBOData.m);
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_useAlbedoTexture, l_renderPack.m_GPassTextureUBOData.useAlbedoTexture);
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_albedo, l_renderPack.m_GPassTextureUBOData.albedo.x, l_renderPack.m_GPassTextureUBOData.albedo.y, l_renderPack.m_GPassTextureUBOData.albedo.z, l_renderPack.m_GPassTextureUBOData.albedo.w);

				drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);
			}
			l_copy.pop();
		}
	}

	// draw conv pass
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_convPassSPC);
	updateUniform(GLEnvironmentRenderPassComponent::get().m_convPass_uni_p, l_p);

	auto l_convPassTDC = GLEnvironmentRenderPassComponent::get().m_convPassTDC;
	auto l_convPassGLTDC = GLEnvironmentRenderPassComponent::get().m_convPassGLTDC;

	activateTexture(l_capturePassGLTDC, 1);

	for (unsigned int i = 0; i < 6; ++i)
	{
		updateUniform(GLEnvironmentRenderPassComponent::get().m_convPass_uni_r, l_v[i]);
		attachTextureToFramebuffer(l_convPassTDC, l_convPassGLTDC, l_FBC, 0, i, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawMesh(l_MDC);
	}

	// draw pre-filter pass
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_preFilterPassSPC);
	updateUniform(GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_p, l_p);

	auto l_preFilterPassTDC = GLEnvironmentRenderPassComponent::get().m_preFilterPassTDC;
	auto l_preFilterPassGLTDC = GLEnvironmentRenderPassComponent::get().m_preFilterPassGLTDC;

	activateTexture(l_convPassGLTDC, 2);

	unsigned int l_maxMipLevels = 5;
	for (unsigned int mip = 0; mip < l_maxMipLevels; ++mip)
	{
		// resize framebuffer according to mip-level size.	
		unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
		unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(l_maxMipLevels - 1);
		updateUniform(GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_roughness, roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			updateUniform(GLEnvironmentRenderPassComponent::get().m_preFilterPass_uni_r, l_v[i]);
			attachTextureToFramebuffer(l_preFilterPassTDC, l_preFilterPassGLTDC, l_FBC, 0, i, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
	}
}