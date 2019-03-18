#include "GLRenderingSystemUtilities.h"
#include "GLEnvironmentRenderingPassUtilities.h"
#include "../../component/GLEnvironmentRenderPassComponent.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/GLFinalRenderPassComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentRenderingPassUtilities
{
	void initializeBRDFLUTPass();
	void initializeEnvironmentCapturePass();
	void initializeGIPass();
	void initializeVoxelizationPass();
	void initializeIrradianceInjectionPass();

	void updateBRDFLUTPass();
	void updateEnvironmentCapturePass();
	void updateGIPass();

	EntityID m_entityID;

	TextureDataDesc BRDFLUTSplitSummingTextureDesc = TextureDataDesc();
	TextureDataDesc BRDFLUTAverangeMSTextureDesc = TextureDataDesc();
	TextureDataDesc EnvCapPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvConvPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvPreFilterPassTextureDesc = TextureDataDesc();
	TextureDataDesc VoxelizationPassTextureDesc = TextureDataDesc();

	GLFrameBufferComponent* GIFBC;
	GLShaderProgramComponent* m_VoxelizationPassSPC;

	GLuint m_VoxelizationPass_uni_m;
	std::vector<GLuint> m_VoxelizationPass_uni_VP;
	std::vector<GLuint> m_VoxelizationPass_uni_VP_inv;
	GLuint m_VoxelizationPass_uni_volumeDimension;
	GLuint m_VoxelizationPass_uni_voxelScale;
	GLuint m_VoxelizationPass_uni_worldMinPoint;

	std::vector<mat4> m_VP;
	std::vector<mat4> m_VP_inv;
	int m_volumeDimension = 256;
	float m_volumeGridSize;

	TextureDataComponent* m_VoxelizationPassTDC_Albedo;
	GLTextureDataComponent* m_VoxelizationPassGLTDC_Albedo;
	TextureDataComponent* m_VoxelizationPassTDC_Normal;
	GLTextureDataComponent* m_VoxelizationPassGLTDC_Normal;

	GLShaderProgramComponent* m_IrradianceInjectionPassSPC;

	GLuint m_IrradianceInjectionPass_uni_dirLight_direction;
	GLuint m_IrradianceInjectionPass_uni_dirLight_luminance;
	GLuint m_IrradianceInjectionPass_uni_volumeDimension;
	GLuint m_IrradianceInjectionPass_uni_voxelSize;
	GLuint m_IrradianceInjectionPass_uni_voxelScale;
	GLuint m_IrradianceInjectionPass_uni_worldMinPoint;

	TextureDataComponent* m_IrradianceInjectionPass_TDC;
	GLTextureDataComponent* m_IrradianceInjectionPassGLTDC;
}

void GLEnvironmentRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();

	BRDFLUTSplitSummingTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	BRDFLUTSplitSummingTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	BRDFLUTSplitSummingTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	BRDFLUTSplitSummingTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	BRDFLUTSplitSummingTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTSplitSummingTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTSplitSummingTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	BRDFLUTSplitSummingTextureDesc.textureWidth = 512;
	BRDFLUTSplitSummingTextureDesc.textureHeight = 512;
	BRDFLUTSplitSummingTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	BRDFLUTAverangeMSTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	BRDFLUTAverangeMSTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	BRDFLUTAverangeMSTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RG16F;
	BRDFLUTAverangeMSTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RG;
	BRDFLUTAverangeMSTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTAverangeMSTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	BRDFLUTAverangeMSTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	BRDFLUTAverangeMSTextureDesc.textureWidth = 512;
	BRDFLUTAverangeMSTextureDesc.textureHeight = 512;
	BRDFLUTAverangeMSTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvCapPassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	EnvCapPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	EnvCapPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvCapPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvCapPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	EnvCapPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvCapPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvCapPassTextureDesc.textureWidth = 2048;
	EnvCapPassTextureDesc.textureHeight = 2048;
	EnvCapPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvConvPassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	EnvConvPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	EnvConvPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvConvPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvConvPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	EnvConvPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvConvPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvConvPassTextureDesc.textureWidth = 128;
	EnvConvPassTextureDesc.textureHeight = 128;
	EnvConvPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	EnvPreFilterPassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	EnvPreFilterPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	EnvPreFilterPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	EnvPreFilterPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	EnvPreFilterPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	EnvPreFilterPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	EnvPreFilterPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	EnvPreFilterPassTextureDesc.textureWidth = 128;
	EnvPreFilterPassTextureDesc.textureHeight = 128;
	EnvPreFilterPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	VoxelizationPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_3D;
	VoxelizationPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	VoxelizationPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA8;
	VoxelizationPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	VoxelizationPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	VoxelizationPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	VoxelizationPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	VoxelizationPassTextureDesc.textureWidth = m_volumeDimension;
	VoxelizationPassTextureDesc.textureHeight = m_volumeDimension;
	VoxelizationPassTextureDesc.textureDepth = m_volumeDimension;
	VoxelizationPassTextureDesc.texturePixelDataType = TexturePixelDataType::UNSIGNED_BYTE;

	initializeBRDFLUTPass();
	initializeEnvironmentCapturePass();
	initializeGIPass();
}

void GLEnvironmentRenderingPassUtilities::initializeBRDFLUTPass()
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

	attachColorRT(l_GLTDC, l_FBC, 0);

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = BRDFLUTAverangeMSTextureDesc;
	l_TDC->m_textureData = { nullptr };

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC = l_GLTDC;

	attachColorRT(l_GLTDC, l_FBC, 1);

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
	m_ShaderFilePaths.m_VSPath = "GL//BRDFLUTPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL//BRDFLUTPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassSPC = rhs;

	////
	m_ShaderFilePaths.m_VSPath = "GL//BRDFLUTMSPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL//BRDFLUTMSPassFragment.sf";

	rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT = getUniformLocation(
		rhs->m_program,
		"uni_brdfLUT");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT,
		0);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC = rhs;
}

void GLEnvironmentRenderingPassUtilities::initializeEnvironmentCapturePass()
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
	auto rhs = addGLShaderProgramComponent(m_entityID);
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
	rhs = addGLShaderProgramComponent(m_entityID);
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
	rhs = addGLShaderProgramComponent(m_entityID);
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

void GLEnvironmentRenderingPassUtilities::initializeGIPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_volumeDimension, m_volumeDimension);

	GIFBC = l_FBC;

	initializeVoxelizationPass();

	initializeIrradianceInjectionPass();
}

void GLEnvironmentRenderingPassUtilities::initializeVoxelizationPass()
{
	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();
	l_TDC->m_textureDataDesc = VoxelizationPassTextureDesc;
	l_TDC->m_textureData = { nullptr };
	m_VoxelizationPassTDC_Albedo = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);
	m_VoxelizationPassGLTDC_Albedo = l_GLTDC;

	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();
	l_TDC->m_textureDataDesc = VoxelizationPassTextureDesc;
	l_TDC->m_textureData = { nullptr };
	m_VoxelizationPassTDC_Normal = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);
	m_VoxelizationPassGLTDC_Normal = l_GLTDC;

	// shader programs and shaders
	ShaderFilePaths m_VoxelizationPassShaderFilePaths;

	////
	m_VoxelizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelizationPassVertex.sf";
	m_VoxelizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelizationPassGeometry.sf";
	m_VoxelizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelizationPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_VoxelizationPassShaderFilePaths);

	m_VoxelizationPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_VoxelizationPass_uni_VP.reserve(3);
	m_VoxelizationPass_uni_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_VoxelizationPass_uni_VP.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP[" + std::to_string(i) + "]")
		);
		m_VoxelizationPass_uni_VP_inv.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP_inv[" + std::to_string(i) + "]")
		);
	}

	m_VoxelizationPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_VoxelizationPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_VoxelizationPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_VoxelizationPassSPC = rhs;

	m_VP.reserve(3);
	m_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_VP.emplace_back();
		m_VP_inv.emplace_back();
	}
}

void GLEnvironmentRenderingPassUtilities::initializeIrradianceInjectionPass()
{
	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();
	l_TDC->m_textureDataDesc = VoxelizationPassTextureDesc;
	l_TDC->m_textureData = { nullptr };
	m_IrradianceInjectionPass_TDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);
	m_IrradianceInjectionPassGLTDC = l_GLTDC;

	ShaderFilePaths m_IrradianceInjectionPassShaderFilePaths;

	////
	m_IrradianceInjectionPassShaderFilePaths.m_CSPath = "GL//GIIrradianceInjectionPassCompute.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_IrradianceInjectionPassShaderFilePaths);

	std::vector<std::string> l_textureNames = { "voxelAlbedo", "voxelNormal" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

	m_IrradianceInjectionPass_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_IrradianceInjectionPass_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");
	m_IrradianceInjectionPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_IrradianceInjectionPass_uni_voxelSize = getUniformLocation(
		rhs->m_program,
		"uni_voxelSize");
	m_IrradianceInjectionPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_IrradianceInjectionPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_IrradianceInjectionPassSPC = rhs;
}

void GLEnvironmentRenderingPassUtilities::update()
{
	updateBRDFLUTPass();
	updateEnvironmentCapturePass();
	updateGIPass();
}

void GLEnvironmentRenderingPassUtilities::updateBRDFLUTPass()
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
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawMesh(l_MDC);

	// draw averange RsF1 LUT
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	activateTexture(GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC, 0);
	drawMesh(l_MDC);
}

void GLEnvironmentRenderingPassUtilities::updateEnvironmentCapturePass()
{
	// bind to framebuffer
	auto l_FBC = GLEnvironmentRenderPassComponent::get().m_capturePassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
	glViewport(0, 0, 2048, 2048);

	auto l_mainCapture = g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>()[0];
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

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	// draw sky
	// @TODO: optimize
	if (l_renderingConfig.drawSky)
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
			l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
			l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);

		for (unsigned int i = 0; i < 6; ++i)
		{
			updateUniform(GLFinalRenderPassComponent::get().m_skyPass_uni_r, l_v[i]);
			attachCubemapColorRT(l_capturePassGLTDC, l_FBC, 0, i, 0);
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
		// @TODO: not this queue, need another culled version from capture component view
		auto l_copy = GLRenderingSystemComponent::get().m_opaquePassDataQueue;
		updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_v, l_v[i]);
		attachCubemapColorRT(l_capturePassGLTDC, l_FBC, 0, i, 0);
		while (l_copy.size() > 0)
		{
			auto l_renderPack = l_copy.front();
			if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
			{
				if (l_renderPack.textureUBOData.useAlbedoTexture)
				{
					activateTexture(l_renderPack.albedoGLTDC, 0);
				}
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_m, l_renderPack.meshUBOData.m);
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_useAlbedoTexture, l_renderPack.textureUBOData.useAlbedoTexture);
				updateUniform(GLEnvironmentRenderPassComponent::get().m_capturePass_uni_albedo, l_renderPack.textureUBOData.albedo.x, l_renderPack.textureUBOData.albedo.y, l_renderPack.textureUBOData.albedo.z, l_renderPack.textureUBOData.albedo.w);

				drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
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
		attachCubemapColorRT(l_convPassGLTDC, l_FBC, 0, i, 0);
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
			attachCubemapColorRT(l_preFilterPassGLTDC, l_FBC, 0, i, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
	}
}

void GLEnvironmentRenderingPassUtilities::updateGIPass()
{
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend * 2.0f;
	auto center = l_sceneAABB.m_center;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;
	auto l_halfSize = m_volumeGridSize / 2.0f;

	// projection matrices
	auto l_p = InnoMath::generateOrthographicMatrix(-l_halfSize, l_halfSize, -l_halfSize, l_halfSize, 0.0f, m_volumeGridSize);

	// view matrices
	m_VP[0] = InnoMath::lookAt(center + vec4(l_halfSize, 0.0f, 0.0f, 0.0f),
		center, vec4(0.0f, 1.0f, 0.0f, 0.0f));
	m_VP[1] = InnoMath::lookAt(center + vec4(0.0f, l_halfSize, 0.0f, 0.0f),
		center, vec4(0.0f, 0.0f, -1.0f, 0.0f));
	m_VP[2] = InnoMath::lookAt(center + vec4(0.0f, 0.0f, l_halfSize, 0.0f),
		center, vec4(0.0f, 1.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < 3; i++)
	{
		m_VP[i] = m_VP[i] * l_p;
		m_VP_inv[i] = m_VP[i].inverse();
	}

	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	// bind to framebuffer
	auto l_FBC = GIFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_volumeDimension, m_volumeDimension);
	glViewport(0, 0, m_volumeDimension, m_volumeDimension);

	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, m_VoxelizationPassGLTDC_Albedo->m_TAO, 0, 0);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_3D, m_VoxelizationPassGLTDC_Normal->m_TAO, 0, 0);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_3D, m_IrradianceInjectionPassGLTDC->m_TAO, 0, 0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glDepthMask(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// voxelization pass
	glBindImageTexture(0, m_VoxelizationPassGLTDC_Albedo->m_TAO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);
	glBindImageTexture(1, m_VoxelizationPassGLTDC_Normal->m_TAO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);

	activateShaderProgram(m_VoxelizationPassSPC);

	updateUniform(m_VoxelizationPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_VoxelizationPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_VoxelizationPass_uni_worldMinPoint, l_sceneAABB.m_boundMin.x, l_sceneAABB.m_boundMin.y, l_sceneAABB.m_boundMin.z);

	for (size_t i = 0; i < m_VP.size(); i++)
	{
		updateUniform(m_VoxelizationPass_uni_VP[i], m_VP[i]);
		updateUniform(m_VoxelizationPass_uni_VP_inv[i], m_VP_inv[i]);
	}

	for (auto& l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE && l_visibleComponent->m_objectStatus == ObjectStatus::ALIVE)
		{
			updateUniform(
				m_VoxelizationPass_uni_m,
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);

			// draw each graphic data of visibleComponent
			for (auto& l_modelPair : l_visibleComponent->m_modelMap)
			{
				// draw meshes
				auto l_MDC = l_modelPair.first;
				if (l_MDC)
				{
					// draw meshes
					drawMesh(l_MDC);
				}
			}
		}
	}

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// irradiance injection pass
	glBindImageTexture(0, m_VoxelizationPassGLTDC_Albedo->m_TAO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32UI);
	glBindImageTexture(1, m_VoxelizationPassGLTDC_Normal->m_TAO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(2, m_IrradianceInjectionPassGLTDC->m_TAO, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

	activateShaderProgram(m_IrradianceInjectionPassSPC);

	updateUniform(
		m_IrradianceInjectionPass_uni_dirLight_direction,
		l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);
	updateUniform(
		m_IrradianceInjectionPass_uni_dirLight_luminance,
		l_sunDataPack.luminance.x, l_sunDataPack.luminance.y, l_sunDataPack.luminance.z);

	updateUniform(m_IrradianceInjectionPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_IrradianceInjectionPass_uni_voxelSize, m_volumeGridSize);
	updateUniform(m_IrradianceInjectionPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_IrradianceInjectionPass_uni_worldMinPoint, l_sceneAABB.m_boundMin.x, l_sceneAABB.m_boundMin.y, l_sceneAABB.m_boundMin.z);

	auto l_workGroups = static_cast<unsigned>(std::ceil(m_volumeDimension / 8.0f));
	glDispatchCompute(l_workGroups, l_workGroups, l_workGroups);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// reset status
	glDepthMask(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}