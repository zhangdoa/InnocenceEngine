#include "GLLightCullingPass.h"
#include "GLRenderingSystemUtilities.h"

#include "GLEarlyZPass.h"

#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLLightCullingPass
{
	bool initializeShaders();
	bool createGridFrustumsBuffer();
	bool createLightIndexCountBuffer();
	bool createLightIndexListBuffer();
	bool createLightGridGLTDC();
	bool createDebugGLTDC();

	bool calculateFrustums();
	bool cullLights();

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_tileFrustumGLSPC;
	GLShaderProgramComponent* m_lightCullingGLSPC;

	ShaderFilePaths m_tileFrustumShaderFilePaths = { "", "", "", "", "", "GL//tileFrustum.comp/" };
	ShaderFilePaths m_lightCullingShaderFilePaths = { "", "", "", "", "", "GL//lightCulling.comp/" };

	GLTextureDataComponent* m_lightGridGLTDC;
	GLTextureDataComponent* m_debugGLTDC;

	EntityID m_entityID;
	const unsigned int m_tileSize = 16;
	const unsigned int m_numThreadPerGroup = 16;
	TVec4<unsigned int> m_tileFrustumNumThreads;
	TVec4<unsigned int> m_tileFrustumNumThreadGroups;
	TVec4<unsigned int> m_lightCullingNumThreads;
	TVec4<unsigned int> m_lightCullingNumThreadGroups;
}

bool GLLightCullingPass::createGridFrustumsBuffer()
{
	auto l_viewportSize = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_tileFrustumNumThreads = TVec4<unsigned int>((unsigned int)l_numThreadsX, (unsigned int)l_numThreadsY, 1, 0);
	m_tileFrustumNumThreadGroups = TVec4<unsigned int>((unsigned int)l_numThreadGroupsX, (unsigned int)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_tileFrustumNumThreads.x * m_tileFrustumNumThreads.y;

	GLRenderingSystemComponent::get().m_gridFrustumsSSBO = generateSSBO(64 * l_elementCount, 0);

	return true;
}

bool GLLightCullingPass::createLightIndexCountBuffer()
{
	auto l_initialIndexCount = 1;

	GLRenderingSystemComponent::get().m_lightListIndexCounterSSBO = generateSSBO(sizeof(unsigned int), 1);

	return true;
}

bool GLLightCullingPass::createLightIndexListBuffer()
{
	auto l_averangeOverlapLight = 64;

	auto l_viewportSize = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_lightCullingNumThreadGroups = TVec4<unsigned int>((unsigned int)l_numThreadGroupsX, (unsigned int)l_numThreadGroupsY, 1, 0);
	m_lightCullingNumThreads = TVec4<unsigned int>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_lightCullingNumThreadGroups.x * m_lightCullingNumThreadGroups.y * l_averangeOverlapLight;

	GLRenderingSystemComponent::get().m_lightIndexListSSBO = generateSSBO(sizeof(unsigned int) * l_elementCount, 2);

	return true;
}

bool GLLightCullingPass::createLightGridGLTDC()
{
	m_lightGridGLTDC = addGLTextureDataComponent();
	m_lightGridGLTDC->m_textureDataDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc.RTDesc;
	m_lightGridGLTDC->m_textureDataDesc.width = m_lightCullingNumThreadGroups.x;
	m_lightGridGLTDC->m_textureDataDesc.height = m_lightCullingNumThreadGroups.y;
	m_lightGridGLTDC->m_textureDataDesc.usageType = TextureUsageType::RAW_IMAGE;
	m_lightGridGLTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_lightGridGLTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	m_lightGridGLTDC->m_textureData = { nullptr };
	initializeGLTextureDataComponent(m_lightGridGLTDC);

	return true;
}

bool GLLightCullingPass::createDebugGLTDC()
{
	m_debugGLTDC = addGLTextureDataComponent();
	m_debugGLTDC->m_textureDataDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc.RTDesc;
	m_debugGLTDC->m_textureDataDesc.usageType = TextureUsageType::RAW_IMAGE;
	m_debugGLTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_debugGLTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	m_debugGLTDC->m_textureData = { nullptr };
	initializeGLTextureDataComponent(m_debugGLTDC);

	return true;
}

bool GLLightCullingPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "LightCullingGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	createGridFrustumsBuffer();

	createLightIndexListBuffer();
	createLightGridGLTDC();
	createDebugGLTDC();

	return true;
}

bool GLLightCullingPass::initializeShaders()
{
	m_tileFrustumGLSPC = addGLShaderProgramComponent(m_entityID);
	m_lightCullingGLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(m_tileFrustumGLSPC, m_tileFrustumShaderFilePaths);
	initializeGLShaderProgramComponent(m_lightCullingGLSPC, m_lightCullingShaderFilePaths);

	return true;
}

bool GLLightCullingPass::calculateFrustums()
{
	RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData.numThreadGroups = m_tileFrustumNumThreadGroups;
	RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData.numThreads = m_tileFrustumNumThreads;
	updateUBO(GLRenderingSystemComponent::get().m_dispatchParamsUBO, RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData);

	activateShaderProgram(m_tileFrustumGLSPC);

	glDispatchCompute(m_tileFrustumNumThreadGroups.x, m_tileFrustumNumThreadGroups.y, m_tileFrustumNumThreadGroups.z);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	return true;
}

bool GLLightCullingPass::cullLights()
{
	RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData.numThreadGroups = m_lightCullingNumThreadGroups;
	RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData.numThreads = m_lightCullingNumThreads;
	updateUBO(GLRenderingSystemComponent::get().m_dispatchParamsUBO, RenderingFrontendSystemComponent::get().m_dispatchParamsGPUData);

	updateSSBO(GLRenderingSystemComponent::get().m_lightListIndexCounterSSBO, 0);

	activateShaderProgram(m_lightCullingGLSPC);

	activateTexture(GLEarlyZPass::getGLRPC()->m_GLTDCs[0], 0);

	glBindImageTexture(1, m_lightGridGLTDC->m_TO, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(2, m_debugGLTDC->m_TO, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	glDispatchCompute(m_lightCullingNumThreadGroups.x, m_lightCullingNumThreadGroups.y, m_lightCullingNumThreadGroups.z);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	return true;
}

bool GLLightCullingPass::update()
{
	activateRenderPass(m_GLRPC);

	createLightIndexCountBuffer();

	calculateFrustums();
	cullLights();

	return true;
}

bool GLLightCullingPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	return true;
}

bool GLLightCullingPass::reloadShader()
{
	return true;
}

GLRenderPassComponent * GLLightCullingPass::getGLRPC()
{
	return m_GLRPC;
}

GLTextureDataComponent* GLLightCullingPass::getLightGrid()
{
	return m_lightGridGLTDC;
}

GLTextureDataComponent * GLLightCullingPass::getHeatMap()
{
	return m_debugGLTDC;
}