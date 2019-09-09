#include "GLLightCullingPass.h"
#include "GLRenderingBackendUtilities.h"

#include "GLEarlyZPass.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

namespace GLLightCullingPass
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

	ShaderFilePaths m_tileFrustumShaderFilePaths = { "", "", "", "", "", "tileFrustum.comp/" };
	ShaderFilePaths m_lightCullingShaderFilePaths = { "", "", "", "", "", "lightCulling.comp/" };

	GLTextureDataComponent* m_lightGridGLTDC;
	GLTextureDataComponent* m_debugGLTDC;

	EntityID m_entityID;
	const uint32_t m_tileSize = 16;
	const uint32_t m_numThreadPerGroup = 16;
	TVec4<uint32_t> m_tileFrustumNumThreads;
	TVec4<uint32_t> m_tileFrustumNumThreadGroups;
	TVec4<uint32_t> m_lightCullingNumThreads;
	TVec4<uint32_t> m_lightCullingNumThreadGroups;
}

bool GLLightCullingPass::createGridFrustumsBuffer()
{
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_tileFrustumNumThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_tileFrustumNumThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_tileFrustumNumThreads.x * m_tileFrustumNumThreads.y;

	GLRenderingBackendComponent::get().m_gridFrustumsSSBO = generateSSBO(64 * l_elementCount, 0, "gridFrustumsSSBO");

	return true;
}

bool GLLightCullingPass::createLightIndexCountBuffer()
{
	auto l_initialIndexCount = 1;

	GLRenderingBackendComponent::get().m_lightListIndexCounterSSBO = generateSSBO(sizeof(uint32_t), 1, "lightListIndexCounterSSBO");

	return true;
}

bool GLLightCullingPass::createLightIndexListBuffer()
{
	auto l_averangeOverlapLight = 64;

	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_lightCullingNumThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);
	m_lightCullingNumThreads = TVec4<uint32_t>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_lightCullingNumThreadGroups.x * m_lightCullingNumThreadGroups.y * l_averangeOverlapLight;

	GLRenderingBackendComponent::get().m_lightIndexListSSBO = generateSSBO(sizeof(uint32_t) * l_elementCount, 2, "lightIndexListSSBO");

	return true;
}

bool GLLightCullingPass::createLightGridGLTDC()
{
	m_lightGridGLTDC = addGLTextureDataComponent();
	m_lightGridGLTDC->m_textureDataDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
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
	m_debugGLTDC->m_textureDataDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
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
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
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
	DispatchParamsGPUData l_dispatchParamsGPUData;
	l_dispatchParamsGPUData.numThreadGroups = m_tileFrustumNumThreadGroups;
	l_dispatchParamsGPUData.numThreads = m_tileFrustumNumThreads;
	updateUBO(GLRenderingBackendComponent::get().m_dispatchParamsUBO, l_dispatchParamsGPUData);

	activateShaderProgram(m_tileFrustumGLSPC);

	glDispatchCompute(m_tileFrustumNumThreadGroups.x, m_tileFrustumNumThreadGroups.y, m_tileFrustumNumThreadGroups.z);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	return true;
}

bool GLLightCullingPass::cullLights()
{
	DispatchParamsGPUData l_dispatchParamsGPUData;
	l_dispatchParamsGPUData.numThreadGroups = m_lightCullingNumThreadGroups;
	l_dispatchParamsGPUData.numThreads = m_lightCullingNumThreads;
	updateUBO(GLRenderingBackendComponent::get().m_dispatchParamsUBO, l_dispatchParamsGPUData);

	updateSSBO(GLRenderingBackendComponent::get().m_lightListIndexCounterSSBO, 0);

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
	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	createLightIndexCountBuffer();
	calculateFrustums();
	cullLights();
	glDeleteBuffers(1, &GLRenderingBackendComponent::get().m_lightListIndexCounterSSBO);

	return true;
}

bool GLLightCullingPass::resize(uint32_t newSizeX, uint32_t newSizeY)
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