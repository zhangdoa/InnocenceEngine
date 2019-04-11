#include "GLEnvironmentRenderPass.h"

#include "GLRenderingSystemUtilities.h"

#include "../../component/GLRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentRenderPass
{
	void initializeBRDFLUTPass();
	void initializeEnvironmentCapturePass();

	void initializeGIPass();
	void initializeVoxelizationPass();
	void initializeIrradianceInjectionPass();
	void initializeVoxelVisualizationPass();

	void updateBRDFLUTPass();
	void updateEnvironmentCapturePass();
	void updateVoxelizationPass();
	void updateIrradianceInjectionPass();
	void updateVoxelVisualizationPass();

	EntityID m_entityID;

	std::function<void()> f_sceneLoadingFinishCallback;

	bool m_isBaked = false;

	GLRenderPassComponent* m_BRDFSplitSumLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFSplitSumLUTPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_BRDFSplitSumLUTPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_BRDFSplitSumLUTPassSPC;

	GLRenderPassComponent* m_BRDFMSAverageLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFMSAverageLUTPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_BRDFMSAverageLUTPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_BRDFMSAverageLUTPassSPC;

	GLRenderPassComponent* m_capturePassGLRPC;
	GLFrameBufferDesc m_capturePassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_capturePassTextureDesc = TextureDataDesc();
	ShaderFilePaths m_capturePassShaderFilePaths = { "GL//environmentCapturePassVertex.sf" , "", "GL//environmentCapturePassFragment.sf" };
	GLShaderProgramComponent* m_capturePassSPC;
	GLuint m_capturePass_uni_albedoTexture;
	GLuint m_capturePass_uni_p;
	GLuint m_capturePass_uni_v;
	GLuint m_capturePass_uni_m;
	GLuint m_capturePass_uni_useAlbedoTexture;
	GLuint m_capturePass_uni_albedo;

	GLRenderPassComponent* m_convPassGLRPC;
	GLFrameBufferDesc m_convPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_convPassTextureDesc = TextureDataDesc();
	ShaderFilePaths m_convPassShaderFilePaths = { "GL//environmentConvolutionPassVertex.sf" , "", "GL//environmentConvolutionPassFragment.sf" };
	GLShaderProgramComponent* m_convPassSPC;
	GLuint m_convPass_uni_capturedCubeMap;
	GLuint m_convPass_uni_p;
	GLuint m_convPass_uni_r;

	GLRenderPassComponent* m_preFilterPassGLRPC;
	GLFrameBufferDesc m_preFilterPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_preFilterPassTextureDesc = TextureDataDesc();
	ShaderFilePaths m_preFilterPassShaderFilePaths = { "GL//environmentPreFilterPassVertex.sf" , "", "GL//environmentPreFilterPassFragment.sf" };
	GLShaderProgramComponent* m_preFilterPassSPC;
	GLuint m_preFilterPass_uni_capturedCubeMap;
	GLuint m_preFilterPass_uni_p;
	GLuint m_preFilterPass_uni_r;
	GLuint m_preFilterPass_uni_roughness;

	GLRenderPassComponent* m_voxelizationPassGLRPC;
	GLFrameBufferDesc m_voxelizationPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_voxelizationPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_voxelizationPassSPC;
	GLuint m_voxelizationPass_uni_m;
	std::vector<GLuint> m_voxelizationPass_uni_VP;
	std::vector<GLuint> m_voxelizationPass_uni_VP_inv;
	GLuint m_voxelizationPass_uni_volumeDimension;
	GLuint m_voxelizationPass_uni_voxelScale;
	GLuint m_voxelizationPass_uni_worldMinPoint;

	std::vector<mat4> m_VP;
	std::vector<mat4> m_VP_inv;
	unsigned int m_volumeDimension = 128;
	unsigned int m_voxelCount = m_volumeDimension * m_volumeDimension * m_volumeDimension;
	float m_volumeGridSize;

	GLRenderPassComponent* m_irradianceInjectionPassGLRPC;
	GLFrameBufferDesc m_irradianceInjectionPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_irradianceInjectionPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_irradianceInjectionPassSPC;
	GLuint m_irradianceInjectionPass_uni_dirLight_direction;
	GLuint m_irradianceInjectionPass_uni_dirLight_luminance;
	GLuint m_irradianceInjectionPass_uni_volumeDimension;
	GLuint m_irradianceInjectionPass_uni_voxelSize;
	GLuint m_irradianceInjectionPass_uni_voxelScale;
	GLuint m_irradianceInjectionPass_uni_worldMinPoint;

	GLRenderPassComponent* m_voxelVisualizationGLRPC;
	GLShaderProgramComponent* m_voxelVisualizationPassSPC;
	GLuint m_voxelVisualizationPass_uni_p;
	GLuint m_voxelVisualizationPass_uni_r;
	GLuint m_voxelVisualizationPass_uni_t;
	GLuint m_voxelVisualizationPass_uni_m;
	GLuint m_voxelVisualizationPass_uni_volumeDimension;
	GLuint m_voxelVisualizationPass_uni_voxelSize;
	GLuint m_voxelVisualizationPass_uni_worldMinPoint;

	GLuint m_VAO;
}

void GLEnvironmentRenderPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	f_sceneLoadingFinishCallback = [&]() {
		m_isBaked = false;
	};

	g_pCoreSystem->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

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

	m_capturePassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	m_capturePassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_capturePassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	m_capturePassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	m_capturePassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	m_capturePassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	m_capturePassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_capturePassTextureDesc.textureWidth = 2048;
	m_capturePassTextureDesc.textureHeight = 2048;
	m_capturePassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	m_capturePassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_capturePassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_capturePassFrameBufferDesc.sizeX = m_capturePassTextureDesc.textureWidth;
	m_capturePassFrameBufferDesc.sizeY = m_capturePassTextureDesc.textureHeight;
	m_capturePassFrameBufferDesc.drawColorBuffers = false;

	m_convPassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	m_convPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_convPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	m_convPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	m_convPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	m_convPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	m_convPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_convPassTextureDesc.textureWidth = 128;
	m_convPassTextureDesc.textureHeight = 128;
	m_convPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	m_convPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_convPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_convPassFrameBufferDesc.sizeX = m_convPassTextureDesc.textureWidth;
	m_convPassFrameBufferDesc.sizeY = m_convPassTextureDesc.textureHeight;
	m_convPassFrameBufferDesc.drawColorBuffers = false;

	m_preFilterPassTextureDesc.textureSamplerType = TextureSamplerType::CUBEMAP;
	m_preFilterPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_preFilterPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	m_preFilterPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	m_preFilterPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	m_preFilterPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	m_preFilterPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_preFilterPassTextureDesc.textureWidth = 128;
	m_preFilterPassTextureDesc.textureHeight = 128;
	m_preFilterPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	m_preFilterPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_preFilterPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_preFilterPassFrameBufferDesc.sizeX = m_preFilterPassTextureDesc.textureWidth;
	m_preFilterPassFrameBufferDesc.sizeY = m_preFilterPassTextureDesc.textureHeight;
	m_preFilterPassFrameBufferDesc.drawColorBuffers = false;

	m_voxelizationPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_3D;
	m_voxelizationPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_voxelizationPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA8;
	m_voxelizationPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	m_voxelizationPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelizationPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelizationPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_voxelizationPassTextureDesc.textureWidth = m_volumeDimension;
	m_voxelizationPassTextureDesc.textureHeight = m_volumeDimension;
	m_voxelizationPassTextureDesc.textureDepth = m_volumeDimension;
	m_voxelizationPassTextureDesc.texturePixelDataType = TexturePixelDataType::UNSIGNED_BYTE;

	m_voxelizationPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_voxelizationPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	m_voxelizationPassFrameBufferDesc.sizeX = m_voxelizationPassTextureDesc.textureWidth;
	m_voxelizationPassFrameBufferDesc.sizeY = m_voxelizationPassTextureDesc.textureHeight;
	m_voxelizationPassFrameBufferDesc.drawColorBuffers = false;

	m_irradianceInjectionPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_3D;
	m_irradianceInjectionPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_irradianceInjectionPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA8;
	m_irradianceInjectionPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	m_irradianceInjectionPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	m_irradianceInjectionPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	m_irradianceInjectionPassTextureDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_irradianceInjectionPassTextureDesc.textureWidth = m_volumeDimension;
	m_irradianceInjectionPassTextureDesc.textureHeight = m_volumeDimension;
	m_irradianceInjectionPassTextureDesc.textureDepth = m_volumeDimension;
	m_irradianceInjectionPassTextureDesc.texturePixelDataType = TexturePixelDataType::UNSIGNED_BYTE;

	m_irradianceInjectionPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_irradianceInjectionPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	m_irradianceInjectionPassFrameBufferDesc.sizeX = m_irradianceInjectionPassTextureDesc.textureWidth;
	m_irradianceInjectionPassFrameBufferDesc.sizeY = m_irradianceInjectionPassTextureDesc.textureHeight;
	m_irradianceInjectionPassFrameBufferDesc.drawColorBuffers = false;

	initializeBRDFLUTPass();
	initializeEnvironmentCapturePass();
	initializeGIPass();
}

void GLEnvironmentRenderPass::initializeBRDFLUTPass()
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

	updateBRDFLUTPass();
}

void GLEnvironmentRenderPass::initializeEnvironmentCapturePass()
{
	// generate and bind framebuffer
	m_capturePassGLRPC = addGLRenderPassComponent(1, m_capturePassFrameBufferDesc, m_capturePassTextureDesc);
	m_convPassGLRPC = addGLRenderPassComponent(1, m_convPassFrameBufferDesc, m_convPassTextureDesc);
	m_preFilterPassGLRPC = addGLRenderPassComponent(1, m_preFilterPassFrameBufferDesc, m_preFilterPassTextureDesc);

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
}

void GLEnvironmentRenderPass::initializeGIPass()
{
	initializeVoxelizationPass();

	initializeIrradianceInjectionPass();

	initializeVoxelVisualizationPass();
}

void GLEnvironmentRenderPass::initializeVoxelizationPass()
{
	m_voxelizationPassGLRPC = addGLRenderPassComponent(2, m_voxelizationPassFrameBufferDesc, m_voxelizationPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_voxelizationPassShaderFilePaths;

	////
	m_voxelizationPassShaderFilePaths.m_VSPath = "GL//GIvoxelizationPassVertex.sf";
	m_voxelizationPassShaderFilePaths.m_GSPath = "GL//GIvoxelizationPassGeometry.sf";
	m_voxelizationPassShaderFilePaths.m_FSPath = "GL//GIvoxelizationPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelizationPassShaderFilePaths);

	std::vector<std::string> l_textureNames = { "uni_voxelAlbedo", "uni_voxelNormal" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

	m_voxelizationPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_voxelizationPass_uni_VP.reserve(3);
	m_voxelizationPass_uni_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_voxelizationPass_uni_VP.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP[" + std::to_string(i) + "]")
		);
		m_voxelizationPass_uni_VP_inv.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP_inv[" + std::to_string(i) + "]")
		);
	}

	m_voxelizationPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_voxelizationPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_voxelizationPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_voxelizationPassSPC = rhs;

	m_VP.reserve(3);
	m_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_VP.emplace_back();
		m_VP_inv.emplace_back();
	}
}

void GLEnvironmentRenderPass::initializeIrradianceInjectionPass()
{
	m_irradianceInjectionPassGLRPC = addGLRenderPassComponent(1, m_irradianceInjectionPassFrameBufferDesc, m_irradianceInjectionPassTextureDesc);

	ShaderFilePaths m_irradianceInjectionPassShaderFilePaths;

	////
	m_irradianceInjectionPassShaderFilePaths.m_CSPath = "GL//GIirradianceInjectionPassCompute.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_irradianceInjectionPassShaderFilePaths);

	std::vector<std::string> l_textureNames = { "voxelAlbedo", "voxelNormal" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

	m_irradianceInjectionPass_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_irradianceInjectionPass_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");
	m_irradianceInjectionPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_irradianceInjectionPass_uni_voxelSize = getUniformLocation(
		rhs->m_program,
		"uni_voxelSize");
	m_irradianceInjectionPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_irradianceInjectionPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_irradianceInjectionPassSPC = rhs;
}

void GLEnvironmentRenderPass::initializeVoxelVisualizationPass()
{
	// generate and bind framebuffer
	m_voxelVisualizationGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	ShaderFilePaths m_voxelVisualizationPassShaderFilePaths;

	////
	m_voxelVisualizationPassShaderFilePaths.m_VSPath = "GL//GIvoxelVisualizationPassVertex.sf";
	m_voxelVisualizationPassShaderFilePaths.m_GSPath = "GL//GIvoxelVisualizationPassGeometry.sf";
	m_voxelVisualizationPassShaderFilePaths.m_FSPath = "GL//GIvoxelVisualizationPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelVisualizationPassShaderFilePaths);

	std::vector<std::string> l_textureNames = { "uni_voxelTexture" };
	updateTextureUniformLocations(rhs->m_program, l_textureNames);

	m_voxelVisualizationPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_voxelVisualizationPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_voxelVisualizationPass_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	m_voxelVisualizationPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_voxelVisualizationPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_voxelVisualizationPass_uni_voxelSize = getUniformLocation(
		rhs->m_program,
		"uni_voxelSize");
	m_voxelVisualizationPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_voxelVisualizationPassSPC = rhs;

	glGenVertexArrays(1, &m_VAO);
}

void GLEnvironmentRenderPass::update()
{
	if (!m_isBaked)
	{
		updateEnvironmentCapturePass();
		updateVoxelizationPass();
		updateIrradianceInjectionPass();
		m_isBaked = true;
	}
}

void GLEnvironmentRenderPass::draw()
{
	updateVoxelVisualizationPass();
}

void GLEnvironmentRenderPass::updateBRDFLUTPass()
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	activateRenderPass(m_BRDFSplitSumLUTPassGLRPC);

	// draw split-Sum LUT
	activateShaderProgram(m_BRDFSplitSumLUTPassSPC);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawMesh(l_MDC);

	// bind to framebuffer
	activateRenderPass(m_BRDFMSAverageLUTPassGLRPC);

	// draw averange RsF1 LUT
	activateShaderProgram(m_BRDFMSAverageLUTPassSPC);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	activateTexture(m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0], 0);

	drawMesh(l_MDC);
}

void GLEnvironmentRenderPass::updateEnvironmentCapturePass()
{
	activateRenderPass(m_capturePassGLRPC);

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

	auto l_capturePassTDC = m_capturePassGLRPC->m_TDCs[0];
	auto l_capturePassGLTDC = m_capturePassGLRPC->m_GLTDCs[0];

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	// draw sky
	// @TODO: optimize
	if (l_renderingConfig.drawSky)
	{
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LEQUAL);

		//activateShaderProgram(GLFinalRenderPassComponent::get().m_skyPassGLSPC);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_p,
		//	l_p);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize,
		//	2048.0f, 2048.0f);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos,
		//	l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);
		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
		//	l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);

		//for (unsigned int i = 0; i < 6; ++i)
		//{
		//	updateUniform(GLFinalRenderPassComponent::get().m_skyPass_uni_r, l_v[i]);
		//	attachCubemapColorRT(l_capturePassGLTDC, m_capturePassGLRPC, 0, i, 0);
		//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//	drawMesh(l_MDC);
		//}
		//glDisable(GL_CULL_FACE);
		//glDisable(GL_DEPTH_TEST);
	}

	// draw opaque meshes
	activateShaderProgram(m_capturePassSPC);
	updateUniform(m_capturePass_uni_p, l_p);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// @TODO: not this queue, need another culled version from capture component view
		auto l_copy = GLRenderingSystemComponent::get().m_opaquePassDataQueue;
		updateUniform(m_capturePass_uni_v, l_v[i]);
		attachCubemapColorRT(l_capturePassGLTDC, m_capturePassGLRPC, 0, i, 0);
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
	activateRenderPass(m_convPassGLRPC);

	activateShaderProgram(m_convPassSPC);
	updateUniform(m_convPass_uni_p, l_p);

	auto l_convPassTDC = m_convPassGLRPC->m_TDCs[0];
	auto l_convPassGLTDC = m_convPassGLRPC->m_GLTDCs[0];

	activateTexture(l_capturePassGLTDC, 1);

	for (unsigned int i = 0; i < 6; ++i)
	{
		updateUniform(m_convPass_uni_r, l_v[i]);
		attachCubemapColorRT(l_convPassGLTDC, m_convPassGLRPC, 0, i, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawMesh(l_MDC);
	}

	// draw pre-filter pass
	activateRenderPass(m_preFilterPassGLRPC);

	activateShaderProgram(m_preFilterPassSPC);
	updateUniform(m_preFilterPass_uni_p, l_p);

	auto l_preFilterPassTDC = m_preFilterPassGLRPC->m_TDCs[0];
	auto l_preFilterPassGLTDC = m_preFilterPassGLRPC->m_GLTDCs[0];

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
			attachCubemapColorRT(l_preFilterPassGLTDC, m_preFilterPassGLRPC, 0, i, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
	}
}
void GLEnvironmentRenderPass::updateVoxelizationPass()
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
		m_VP[i] = l_p * m_VP[i];
		m_VP_inv[i] = m_VP[i].inverse();
	}

	activateRenderPass(m_voxelizationPassGLRPC);

	// disable status
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glDepthMask(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// voxelization pass
	glBindImageTexture(0, m_voxelizationPassGLRPC->m_GLTDCs[0]->m_TAO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);
	glBindImageTexture(1, m_voxelizationPassGLRPC->m_GLTDCs[1]->m_TAO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);

	activateShaderProgram(m_voxelizationPassSPC);

	updateUniform(m_voxelizationPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_voxelizationPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_voxelizationPass_uni_worldMinPoint, l_sceneAABB.m_boundMin.x, l_sceneAABB.m_boundMin.y, l_sceneAABB.m_boundMin.z);

	for (size_t i = 0; i < m_VP.size(); i++)
	{
		updateUniform(m_voxelizationPass_uni_VP[i], m_VP[i]);
		updateUniform(m_voxelizationPass_uni_VP_inv[i], m_VP_inv[i]);
	}

	for (auto& l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE && l_visibleComponent->m_objectStatus == ObjectStatus::ALIVE)
		{
			updateUniform(
				m_voxelizationPass_uni_m,
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
}

void GLEnvironmentRenderPass::updateIrradianceInjectionPass()
{
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend * 2.0f;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;

	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	activateRenderPass(m_irradianceInjectionPassGLRPC);

	activateTexture(m_voxelizationPassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_voxelizationPassGLRPC->m_GLTDCs[1], 1);
	glBindImageTexture(2, m_irradianceInjectionPassGLRPC->m_GLTDCs[0]->m_TAO, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

	activateShaderProgram(m_irradianceInjectionPassSPC);

	updateUniform(
		m_irradianceInjectionPass_uni_dirLight_direction,
		l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);
	updateUniform(
		m_irradianceInjectionPass_uni_dirLight_luminance,
		l_sunDataPack.luminance.x, l_sunDataPack.luminance.y, l_sunDataPack.luminance.z);

	updateUniform(m_irradianceInjectionPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_irradianceInjectionPass_uni_voxelSize, l_voxelSize);
	updateUniform(m_irradianceInjectionPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_irradianceInjectionPass_uni_worldMinPoint, l_sceneAABB.m_boundMin.x, l_sceneAABB.m_boundMin.y, l_sceneAABB.m_boundMin.z);

	auto l_workGroups = static_cast<unsigned>(std::ceil(m_volumeDimension / 8.0f));
	glDispatchCompute(l_workGroups, l_workGroups, l_workGroups);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// reset status
	glDepthMask(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void GLEnvironmentRenderPass::updateVoxelVisualizationPass()
{
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend * 2.0f;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;

	// voxel visualization pass
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	auto l_p = l_cameraDataPack.p_original;
	auto l_r = l_cameraDataPack.r;
	auto l_t = l_cameraDataPack.t;
	auto l_ms = InnoMath::toScaleMatrix(vec4(l_voxelSize, l_voxelSize, l_voxelSize, 1.0f));
	auto l_mt = InnoMath::toTranslationMatrix(l_sceneAABB.m_boundMin);

	auto l_m = l_mt * l_ms;

	activateRenderPass(m_voxelVisualizationGLRPC);

	activateShaderProgram(m_voxelVisualizationPassSPC);

	updateUniform(m_voxelVisualizationPass_uni_p, l_p);
	updateUniform(m_voxelVisualizationPass_uni_r, l_r);
	updateUniform(m_voxelVisualizationPass_uni_t, l_t);
	updateUniform(m_voxelVisualizationPass_uni_m, l_m);

	updateUniform(m_voxelVisualizationPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_voxelVisualizationPass_uni_voxelSize, l_voxelSize);
	updateUniform(m_voxelVisualizationPass_uni_worldMinPoint, l_sceneAABB.m_boundMin.x, l_sceneAABB.m_boundMin.y, l_sceneAABB.m_boundMin.z);

	glBindImageTexture(0, m_irradianceInjectionPassGLRPC->m_GLTDCs[0]->m_TAO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);

	glBindVertexArray(m_VAO);
	glDrawArrays(GL_POINTS, 0, m_voxelCount);
}

GLTextureDataComponent * GLEnvironmentRenderPass::getBRDFSplitSumLUT()
{
	return m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderPass::getBRDFMSAverageLUT()
{
	return m_BRDFMSAverageLUTPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderPass::getConvPassGLTDC()
{
	return m_convPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderPass::getPreFilterPassGLTDC()
{
	return m_preFilterPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLEnvironmentRenderPass::getVoxelVisualizationPassGLTDC()
{
	return m_voxelVisualizationGLRPC->m_GLTDCs[0];
}