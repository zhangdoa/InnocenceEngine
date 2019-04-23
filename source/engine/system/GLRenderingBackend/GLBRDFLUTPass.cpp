#include "GLBRDFLUTPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLBRDFLUTPass
{
	EntityID m_entityID;

	GLRenderPassComponent* m_BRDFSplitSumLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFSplitSumLUTPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_BRDFSplitSumLUTPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_BRDFSplitSumLUTPassSPC;
	ShaderFilePaths m_BRDFSplitSumShaderFilePaths = { "GL//BRDFLUTPass.vert" , "", "GL//BRDFLUTPass.frag" };

	GLRenderPassComponent* m_BRDFMSAverageLUTPassGLRPC;
	GLFrameBufferDesc m_BRDFMSAverageLUTPassFrameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_BRDFMSAverageLUTPassTextureDesc = TextureDataDesc();
	GLShaderProgramComponent* m_BRDFMSAverageLUTPassSPC;
	ShaderFilePaths m_BRDFMSAverageShaderFilePaths = { "GL//BRDFLUTMSPass.vert" , "", "GL//BRDFLUTMSPass.frag" };
}

bool GLBRDFLUTPass::initialize()
{
	m_BRDFSplitSumLUTPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	m_BRDFSplitSumLUTPassTextureDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	m_BRDFSplitSumLUTPassTextureDesc.colorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	m_BRDFSplitSumLUTPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_BRDFSplitSumLUTPassTextureDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFSplitSumLUTPassTextureDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFSplitSumLUTPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	m_BRDFSplitSumLUTPassTextureDesc.width = 512;
	m_BRDFSplitSumLUTPassTextureDesc.height = 512;
	m_BRDFSplitSumLUTPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	m_BRDFSplitSumLUTPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_BRDFSplitSumLUTPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_BRDFSplitSumLUTPassFrameBufferDesc.sizeX = m_BRDFSplitSumLUTPassTextureDesc.width;
	m_BRDFSplitSumLUTPassFrameBufferDesc.sizeY = m_BRDFSplitSumLUTPassTextureDesc.height;
	m_BRDFSplitSumLUTPassFrameBufferDesc.drawColorBuffers = true;

	m_BRDFMSAverageLUTPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	m_BRDFMSAverageLUTPassTextureDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	m_BRDFMSAverageLUTPassTextureDesc.colorComponentsFormat = TextureColorComponentsFormat::RG16F;
	m_BRDFMSAverageLUTPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::RG;
	m_BRDFMSAverageLUTPassTextureDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFMSAverageLUTPassTextureDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_BRDFMSAverageLUTPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	m_BRDFMSAverageLUTPassTextureDesc.width = 512;
	m_BRDFMSAverageLUTPassTextureDesc.height = 512;
	m_BRDFMSAverageLUTPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	m_BRDFMSAverageLUTPassFrameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_BRDFMSAverageLUTPassFrameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_BRDFMSAverageLUTPassFrameBufferDesc.sizeX = m_BRDFMSAverageLUTPassTextureDesc.width;
	m_BRDFMSAverageLUTPassFrameBufferDesc.sizeY = m_BRDFMSAverageLUTPassTextureDesc.height;
	m_BRDFMSAverageLUTPassFrameBufferDesc.drawColorBuffers = true;

	// generate and bind framebuffer
	m_BRDFSplitSumLUTPassGLRPC = addGLRenderPassComponent(1, m_BRDFSplitSumLUTPassFrameBufferDesc, m_BRDFSplitSumLUTPassTextureDesc);

	m_BRDFMSAverageLUTPassGLRPC = addGLRenderPassComponent(1, m_BRDFMSAverageLUTPassFrameBufferDesc, m_BRDFMSAverageLUTPassTextureDesc);

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_BRDFSplitSumShaderFilePaths);

	m_BRDFSplitSumLUTPassSPC = rhs;

	rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_BRDFMSAverageShaderFilePaths);

	m_BRDFMSAverageLUTPassSPC = rhs;

	update();

	return true;
}

bool GLBRDFLUTPass::update()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// draw split-Sum LUT
	activateRenderPass(m_BRDFSplitSumLUTPassGLRPC);

	activateShaderProgram(m_BRDFSplitSumLUTPassSPC);

	drawMesh(l_MDC);

	// draw averange RsF1 LUT
	activateRenderPass(m_BRDFMSAverageLUTPassGLRPC);

	activateShaderProgram(m_BRDFMSAverageLUTPassSPC);

	activateTexture(m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0], 0);

	drawMesh(l_MDC);

	return true;
}

bool GLBRDFLUTPass::reloadShader()
{
	deleteShaderProgram(m_BRDFSplitSumLUTPassSPC);

	initializeGLShaderProgramComponent(m_BRDFSplitSumLUTPassSPC, m_BRDFSplitSumShaderFilePaths);

	deleteShaderProgram(m_BRDFMSAverageLUTPassSPC);

	initializeGLShaderProgramComponent(m_BRDFMSAverageLUTPassSPC, m_BRDFMSAverageShaderFilePaths);

	return true;
}

GLTextureDataComponent * GLBRDFLUTPass::getBRDFSplitSumLUT()
{
	return m_BRDFSplitSumLUTPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent * GLBRDFLUTPass::getBRDFMSAverageLUT()
{
	return m_BRDFMSAverageLUTPassGLRPC->m_GLTDCs[0];
}