#include "GLRenderingSystemUtilities.h"
#include "GLEnvironmentRenderingPassUtilities.h"
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

	TextureDataDesc EnvCapPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvConvPassTextureDesc = TextureDataDesc();
	TextureDataDesc EnvPreFilterPassTextureDesc = TextureDataDesc();
	TextureDataDesc VoxelizationPassTextureDesc = TextureDataDesc();

	GLRenderPassComponent* m_BRDFSplitSumLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFSplitSumLUTPassFrameBufferDesc = GLFrameBufferDesc();
	GLShaderProgramComponent* m_BRDFSplitSumLUTPassSPC;
	TextureDataDesc m_BRDFSplitSumLUTPassTextureDesc = TextureDataDesc();

	GLRenderPassComponent* m_BRDFMSAverageLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFMSAverageLUTPassFrameBufferDesc = GLFrameBufferDesc();
	GLShaderProgramComponent* m_BRDFMSAverageLUTPassSPC;
	TextureDataDesc m_BRDFMSAverageLUTPassTextureDesc = TextureDataDesc();

	ShaderFilePaths m_capturePassShaderFilePaths = { "GL//environmentCapturePassVertex.sf" , "", "GL//environmentCapturePassFragment.sf" };
	GLShaderProgramComponent* m_capturePassSPC;
	GLuint m_capturePass_uni_albedoTexture;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_v;
	GLuint m_capturePass_uni_m;
	GLuint m_capturePass_uni_useAlbedoTexture;
	GLuint m_capturePass_uni_albedo;
	GLFrameBufferComponent* m_capturePassFBC;
	TextureDataComponent* m_capturePassTDC;
	GLTextureDataComponent* m_capturePassGLTDC;

	ShaderFilePaths m_convPassShaderFilePaths = { "GL//environmentConvolutionPassVertex.sf" , "", "GL//environmentConvolutionPassFragment.sf" };
	GLShaderProgramComponent* m_convPassSPC;
	GLuint m_convPass_uni_capturedCubeMap;
	GLuint m_convPass_uni_p;
	GLuint m_convPass_uni_r;
	TextureDataComponent* m_convPassTDC;
	GLTextureDataComponent* m_convPassGLTDC;

	ShaderFilePaths m_preFilterPassShaderFilePaths = { "GL//environmentPreFilterPassVertex.sf" , "", "GL//environmentPreFilterPassFragment.sf" };
	GLShaderProgramComponent* m_preFilterPassSPC;
	GLuint m_preFilterPass_uni_capturedCubeMap;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;
	GLuint m_preFilterPass_uni_roughness;
	TextureDataComponent* m_preFilterPassTDC;
	GLTextureDataComponent* m_preFilterPassGLTDC;

	GLFrameBufferComponent* m_GIPassFBC;
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

	m_BRDFSplitSumLUTPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	m_BRDFSplitSumLUTPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_BRDFSplitSumLUTPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	m_BRDFSplitSumLUTPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	m_BRDFSplitSumLUTPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFSplitSumLUTPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFSplitSumLUTPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	m_BRDFSplitSumLUTPassTextureDesc.textureWidth = 512;
	m_BRDFSplitSumLUTPassTextureDesc.textureHeight = 512;
	m_BRDFSplitSumLUTPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	m_BRDFSplitSumLUTPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_BRDFSplitSumLUTPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_BRDFSplitSumLUTPassFrameBufferDesc.sizeX = m_BRDFSplitSumLUTPassTextureDesc.textureWidth;
	m_BRDFSplitSumLUTPassFrameBufferDesc.sizeY = m_BRDFSplitSumLUTPassTextureDesc.textureHeight;
	m_BRDFSplitSumLUTPassFrameBufferDesc.drawColorBuffers = true;

	m_BRDFMSAverageLUTPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	m_BRDFMSAverageLUTPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_BRDFMSAverageLUTPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RG16F;
	m_BRDFMSAverageLUTPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RG;
	m_BRDFMSAverageLUTPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFMSAverageLUTPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFMSAverageLUTPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	m_BRDFMSAverageLUTPassTextureDesc.textureWidth = 512;
	m_BRDFMSAverageLUTPassTextureDesc.textureHeight = 512;
	m_BRDFMSAverageLUTPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	m_BRDFMSAverageLUTPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_BRDFMSAverageLUTPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_BRDFMSAverageLUTPassFrameBufferDesc.sizeX = m_BRDFMSAverageLUTPassTextureDesc.textureWidth;
	m_BRDFMSAverageLUTPassFrameBufferDesc.sizeY = m_BRDFMSAverageLUTPassTextureDesc.textureHeight;
	m_BRDFMSAverageLUTPassFrameBufferDesc.drawColorBuffers = true;

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
	auto l_RPC = addGLRenderPassComponent(1, m_BRDFSplitSumLUTPassFrameBufferDesc, m_BRDFSplitSumLUTPassTextureDesc);
	m_BRDFSplitSumLUTPassGLRPC = l_RPC;

	l_RPC = addGLRenderPassComponent(1, m_BRDFMSAverageLUTPassFrameBufferDesc, m_BRDFMSAverageLUTPassTextureDesc);
	m_BRDFMSAverageLUTPassGLRPC = l_RPC;

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	////
	m_ShaderFilePaths.m_VSPath = "GL//BRDFLUTPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL//BRDFLUTPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	m_BRDFSplitSumLUTPassSPC = rhs;

	////
	m_ShaderFilePaths.m_VSPath = "GL//BRDFLUTMSPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL//BRDFLUTMSPassFragment.sf";

	rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	std::vector<std::string> l_textureNames = { "uni_brdfLUT" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

	m_BRDFMSAverageLUTPassSPC = rhs;
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

	m_capturePassFBC = l_FBC;

	// generate and bind texture
	// Capture pass
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvCapPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	m_capturePassTDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	m_capturePassGLTDC = l_GLTDC;

	// Convolution pass
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvConvPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	m_convPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	m_convPassGLTDC = l_GLTDC;

	// Pre-filter pass
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc = EnvPreFilterPassTextureDesc;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	m_preFilterPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	m_preFilterPassGLTDC = l_GLTDC;

	// capture pass shader
	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_capturePassShaderFilePaths);

	m_capturePass_uni_albedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_albedoTexture");
	updateUniform(
		m_capturePass_uni_albedoTexture,
		0);
	m_capturePass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_capturePass_uni_v = getUniformLocation(
		rhs->m_program,
		"uni_v");
	m_capturePass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");
	m_capturePass_uni_useAlbedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_useAlbedoTexture");
	m_capturePass_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");

	m_capturePassSPC = rhs;

	// conv pass shader
	rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_convPassShaderFilePaths);

	m_convPass_uni_capturedCubeMap = getUniformLocation(
		rhs->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		m_convPass_uni_capturedCubeMap,
		0);
	m_convPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_convPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");

	m_convPassSPC = rhs;

	// pre-filter pass shader
	rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_preFilterPassShaderFilePaths);

	m_preFilterPass_uni_capturedCubeMap = getUniformLocation(
		rhs->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		m_preFilterPass_uni_capturedCubeMap,
		0);
	m_preFilterPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_preFilterPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_preFilterPass_uni_roughness = getUniformLocation(
		rhs->m_program,
		"uni_roughness");

	m_preFilterPassSPC = rhs;

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

	m_GIPassFBC = l_FBC;

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

	std::vector<std::string> l_textureNames = { "uni_voxelAlbedo", "uni_voxelNormal" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

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
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = m_BRDFSplitSumLUTPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	// draw split-Sum LUT
	activateShaderProgram(m_BRDFSplitSumLUTPassSPC);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawMesh(l_MDC);

	// bind to framebuffer
	l_FBC = m_BRDFMSAverageLUTPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	// draw averange RsF1 LUT
	activateShaderProgram(m_BRDFMSAverageLUTPassSPC);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	activateTexture(m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0], 0);

	drawMesh(l_MDC);
}

void GLEnvironmentRenderingPassUtilities::updateEnvironmentCapturePass()
{
	// bind to framebuffer
	auto l_FBC = m_capturePassFBC;
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

	auto l_capturePassTDC = m_capturePassTDC;
	auto l_capturePassGLTDC = m_capturePassGLTDC;

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
	activateShaderProgram(m_capturePassSPC);
	updateUniform(m_capturePass_uni_p, l_p);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// @TODO: not this queue, need another culled version from capture component view
		auto l_copy = GLRenderingSystemComponent::get().m_opaquePassDataQueue;
		updateUniform(m_capturePass_uni_v, l_v[i]);
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
				updateUniform(m_capturePass_uni_m, l_renderPack.meshUBOData.m);
				updateUniform(m_capturePass_uni_useAlbedoTexture, l_renderPack.textureUBOData.useAlbedoTexture);
				updateUniform(m_capturePass_uni_albedo, l_renderPack.textureUBOData.albedo.x, l_renderPack.textureUBOData.albedo.y, l_renderPack.textureUBOData.albedo.z, l_renderPack.textureUBOData.albedo.w);

				drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
			}
			l_copy.pop();
		}
	}

	// draw conv pass
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(m_convPassSPC);
	updateUniform(m_convPass_uni_p, l_p);

	auto l_convPassTDC = m_convPassTDC;
	auto l_convPassGLTDC = m_convPassGLTDC;

	activateTexture(l_capturePassGLTDC, 1);

	for (unsigned int i = 0; i < 6; ++i)
	{
		updateUniform(m_convPass_uni_r, l_v[i]);
		attachCubemapColorRT(l_convPassGLTDC, l_FBC, 0, i, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawMesh(l_MDC);
	}

	// draw pre-filter pass
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(m_preFilterPassSPC);
	updateUniform(m_preFilterPass_uni_p, l_p);

	auto l_preFilterPassTDC = m_preFilterPassTDC;
	auto l_preFilterPassGLTDC = m_preFilterPassGLTDC;

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
		updateUniform(m_preFilterPass_uni_roughness, roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			updateUniform(m_preFilterPass_uni_r, l_v[i]);
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
	m_VP[0] = InnoMath::lookAt(center,
		center + vec4(l_halfSize, 0.0f, 0.0f, 0.0f), vec4(0.0f, 1.0f, 0.0f, 0.0f));
	m_VP[1] = InnoMath::lookAt(center,
		center + vec4(0.0f, l_halfSize, 0.0f, 0.0f), vec4(0.0f, 0.0f, -1.0f, 0.0f));
	m_VP[2] = InnoMath::lookAt(center,
		center + vec4(0.0f, 0.0f, l_halfSize, 0.0f), vec4(0.0f, 1.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < 3; i++)
	{
		m_VP[i] = l_p * m_VP[i];
		m_VP_inv[i] = m_VP[i].inverse();
	}

	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	// bind to framebuffer
	auto l_FBC = m_GIPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_volumeDimension, m_volumeDimension);
	glViewport(0, 0, m_volumeDimension, m_volumeDimension);

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
	glBindImageTexture(1, m_VoxelizationPassGLTDC_Normal->m_TAO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32UI);
	glBindImageTexture(2, m_IrradianceInjectionPassGLTDC->m_TAO, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32UI);

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

GLTextureDataComponent * GLEnvironmentRenderingPassUtilities::getBRDFSplitSumLUT()
{
	return m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderingPassUtilities::getBRDFMSAverageLUT()
{
	return m_BRDFMSAverageLUTPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderingPassUtilities::getConvPassGLTDC()
{
	return m_convPassGLTDC;
}

GLTextureDataComponent * GLEnvironmentRenderingPassUtilities::getPreFilterPassGLTDC()
{
	return m_preFilterPassGLTDC;
}