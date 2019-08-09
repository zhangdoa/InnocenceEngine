#include "LightCullingPass.h"
#include "DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace LightCullingPass
{
	RenderPassDataComponent* m_RPDC_Frustum;
	RenderPassDataComponent* m_RPDC_LightCulling;
	ShaderProgramComponent* m_SPC_TileFrustum;
	ShaderProgramComponent* m_SPC_LightCulling;

	GPUBufferDataComponent* m_tileFrustumGBDC;
	GPUBufferDataComponent* m_lightListIndexCounterGBDC;
	GPUBufferDataComponent* m_lightIndexListGBDC;

	TextureDataComponent* m_lightGridTDC;
	TextureDataComponent* m_debugTDC;

	const unsigned int m_tileSize = 16;
	const unsigned int m_numThreadPerGroup = 16;
	TVec4<unsigned int> m_tileFrustumNumThreads;
	TVec4<unsigned int> m_tileFrustumNumThreadGroups;
	TVec4<unsigned int> m_lightCullingNumThreads;
	TVec4<unsigned int> m_lightCullingNumThreadGroups;

	bool createGridFrustumsBuffer();
	bool createLightIndexCounterBuffer();
	bool createLightIndexListBuffer();
	bool createLightGridTDC();
	bool createDebugTDC();
}

bool LightCullingPass::createGridFrustumsBuffer()
{
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_tileFrustumNumThreads = TVec4<unsigned int>((unsigned int)l_numThreadsX, (unsigned int)l_numThreadsY, 1, 0);
	m_tileFrustumNumThreadGroups = TVec4<unsigned int>((unsigned int)l_numThreadGroupsX, (unsigned int)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_tileFrustumNumThreads.x * m_tileFrustumNumThreads.y;

	m_tileFrustumGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("TileFrustumGPUBuffer/");
	m_tileFrustumGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_tileFrustumGBDC->m_ElementCount = l_elementCount;
	m_tileFrustumGBDC->m_ElementSize = 64;
	m_tileFrustumGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_tileFrustumGBDC);

	return true;
}

bool LightCullingPass::createLightIndexCounterBuffer()
{
	auto l_initialIndexCount = 1;

	m_lightListIndexCounterGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("LightListIndexCounterGBDC/");
	m_lightListIndexCounterGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightListIndexCounterGBDC->m_ElementCount = 1;
	m_lightListIndexCounterGBDC->m_ElementSize = sizeof(uint32_t);
	m_lightListIndexCounterGBDC->m_BindingPoint = 1;
	m_lightListIndexCounterGBDC->m_InitialData = &l_initialIndexCount;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_lightListIndexCounterGBDC);

	return true;
}

bool LightCullingPass::createLightIndexListBuffer()
{
	auto l_averangeOverlapLight = 64;

	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_lightCullingNumThreadGroups = TVec4<unsigned int>((unsigned int)l_numThreadGroupsX, (unsigned int)l_numThreadGroupsY, 1, 0);
	m_lightCullingNumThreads = TVec4<unsigned int>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_lightCullingNumThreadGroups.x * m_lightCullingNumThreadGroups.y * l_averangeOverlapLight;

	m_lightIndexListGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("LightIndexListGBDC/");
	m_lightIndexListGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightIndexListGBDC->m_ElementCount = l_elementCount;
	m_lightIndexListGBDC->m_ElementSize = sizeof(uint32_t);
	m_lightIndexListGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_lightIndexListGBDC);

	return true;
}

bool LightCullingPass::createLightGridTDC()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_lightGridTDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("LightGrid/");
	m_lightGridTDC->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_lightGridTDC->m_textureDataDesc.Width = m_lightCullingNumThreadGroups.x;
	m_lightGridTDC->m_textureDataDesc.Height = m_lightCullingNumThreadGroups.y;
	m_lightGridTDC->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
	m_lightGridTDC->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGridTDC->m_textureDataDesc.PixelDataType = TexturePixelDataType::UINT32;

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_lightGridTDC);

	return true;
}

bool LightCullingPass::createDebugTDC()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_debugTDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("LightCullingDebug/");
	m_debugTDC->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_debugTDC->m_textureDataDesc.UsageType = TextureUsageType::RawImage;

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_debugTDC);

	return true;
}

bool LightCullingPass::Setup()
{
	m_SPC_TileFrustum = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("TileFrustum/");
	m_SPC_TileFrustum->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_TileFrustum);

	m_SPC_LightCulling = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("LightCulling/");
	m_SPC_LightCulling->m_ShaderFilePaths.m_CSPath = "lightCulling.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_LightCulling);

	////
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;

	m_RPDC_Frustum = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("ComputePass_TileFrustum/");
	m_RPDC_Frustum->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_Frustum->m_ResourceBinderLayoutDescs.resize(3);
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 7;

	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 8;

	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC_Frustum->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 0;

	m_RPDC_Frustum->m_ShaderProgram = m_SPC_TileFrustum;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_Frustum);

	////
	m_RPDC_LightCulling = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("ComputePass_LightCulling/");
	m_RPDC_LightCulling->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs.resize(10);
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 4;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 7;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 8;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 0;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 1;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 2;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 3;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_IsRanged = true;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_GlobalSlot = 8;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_LocalSlot = 4;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_GlobalSlot = 9;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_LocalSlot = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_IsRanged = true;

	m_RPDC_LightCulling->m_ShaderProgram = m_SPC_LightCulling;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_LightCulling);

	////
	createGridFrustumsBuffer();
	createLightIndexCounterBuffer();
	createLightIndexListBuffer();
	createLightGridTDC();
	createDebugTDC();

	return true;
}

bool LightCullingPass::Initialize()
{
	return true;
}

bool LightCullingPass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_lightListIndexCounter = 1;
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_lightListIndexCounterGBDC, &l_lightListIndexCounter);

	DispatchParamsGPUData l_tileFrustumWorkload;
	l_tileFrustumWorkload.numThreadGroups = m_tileFrustumNumThreadGroups;
	l_tileFrustumWorkload.numThreads = m_tileFrustumNumThreads;

	DispatchParamsGPUData lightCullingWorkload;
	lightCullingWorkload.numThreadGroups = m_lightCullingNumThreadGroups;
	lightCullingWorkload.numThreads = m_lightCullingNumThreads;

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_tileFrustumWorkload, 0, 1);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &lightCullingWorkload, 1, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Frustum, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Frustum);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Frustum);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Frustum, ShaderStage::Compute, l_SkyGBDC->m_ResourceBinder, 0, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Frustum, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 1, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Frustum, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite, 0);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_RPDC_Frustum, m_tileFrustumNumThreadGroups.x, m_tileFrustumNumThreadGroups.y, m_tileFrustumNumThreadGroups.z);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Frustum, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite, 0);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Frustum);

	////
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_LightCulling, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_LightCulling);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_LightCulling);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_PointLightGBDC->m_ResourceBinder, 1, 4, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_SkyGBDC->m_ResourceBinder, 2, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 3, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightListIndexCounterGBDC->m_ResourceBinder, 5, 1, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightIndexListGBDC->m_ResourceBinder, 6, 2, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightGridTDC->m_ResourceBinder, 7, 3, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_debugTDC->m_ResourceBinder, 8, 4, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, OpaquePass::GetRPDC()->m_DepthStencilRenderTarget->m_ResourceBinder, 9, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_RPDC_LightCulling, m_lightCullingNumThreadGroups.x, m_lightCullingNumThreadGroups.y, m_lightCullingNumThreadGroups.z);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightListIndexCounterGBDC->m_ResourceBinder, 5, 1, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightIndexListGBDC->m_ResourceBinder, 6, 2, Accessibility::ReadWrite, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightGridTDC->m_ResourceBinder, 7, 3, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_debugTDC->m_ResourceBinder, 8, 4, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, OpaquePass::GetRPDC()->m_DepthStencilRenderTarget->m_ResourceBinder, 9, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_LightCulling);

	return true;
}

bool LightCullingPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Frustum);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Frustum);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_LightCulling);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_LightCulling);

	return true;
}

bool LightCullingPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_Frustum);

	return true;
}

IResourceBinder * LightCullingPass::GetLightGrid()
{
	return m_lightGridTDC->m_ResourceBinder;
}

GPUBufferDataComponent * LightCullingPass::GetLightIndexList()
{
	return m_lightIndexListGBDC;
}

IResourceBinder * LightCullingPass::GetHeatMap()
{
	return m_debugTDC->m_ResourceBinder;
}