#include "LightCullingPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace LightCullingPass
{
	RenderPassDataComponent* m_RPDC_TileFrustum;
	RenderPassDataComponent* m_RPDC_LightCulling;
	ShaderProgramComponent* m_SPC_TileFrustum;
	ShaderProgramComponent* m_SPC_LightCulling;
	SamplerDataComponent* m_SDC_LightCulling;

	GPUBufferDataComponent* m_tileFrustumGBDC;
	GPUBufferDataComponent* m_lightListIndexCounterGBDC;
	GPUBufferDataComponent* m_lightIndexListGBDC;

	TextureDataComponent* m_lightGridTDC;
	TextureDataComponent* m_heatMapTDC;

	const uint32_t m_tileSize = 16;
	const uint32_t m_numThreadPerGroup = 16;
	TVec4<uint32_t> m_tileFrustumNumThreads;
	TVec4<uint32_t> m_tileFrustumNumThreadGroups;
	TVec4<uint32_t> m_lightCullingNumThreads;
	TVec4<uint32_t> m_lightCullingNumThreadGroups;

	bool createGridFrustumsBuffer();
	bool createLightIndexCounterBuffer();
	bool createLightIndexListBuffer();
	bool createLightGridTDC();
	bool createHeatMapTDC();
}

bool LightCullingPass::createGridFrustumsBuffer()
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadsY = std::ceil(l_viewportSize.y / m_tileSize);

	auto l_numThreadGroupsX = std::ceil(l_numThreadsX / m_numThreadPerGroup);
	auto l_numThreadGroupsY = std::ceil(l_numThreadsY / m_numThreadPerGroup);

	m_tileFrustumNumThreads = TVec4<uint32_t>((uint32_t)l_numThreadsX, (uint32_t)l_numThreadsY, 1, 0);
	m_tileFrustumNumThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);

	auto l_elementCount = m_tileFrustumNumThreads.x * m_tileFrustumNumThreads.y;

	m_tileFrustumGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TileFrustumGPUBuffer/");
	m_tileFrustumGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_tileFrustumGBDC->m_ElementCount = l_elementCount;
	m_tileFrustumGBDC->m_ElementSize = 64;

	return true;
}

bool LightCullingPass::createLightIndexCounterBuffer()
{
	auto l_initialIndexCount = 1;

	m_lightListIndexCounterGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("LightListIndexCounterGBDC/");
	m_lightListIndexCounterGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightListIndexCounterGBDC->m_ElementCount = 1;
	m_lightListIndexCounterGBDC->m_ElementSize = sizeof(uint32_t);
	m_lightListIndexCounterGBDC->m_InitialData = &l_initialIndexCount;

	return true;
}

bool LightCullingPass::createLightIndexListBuffer()
{
	auto l_averangeOverlapLight = 64;

	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_lightCullingNumThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);
	m_lightCullingNumThreads = TVec4<uint32_t>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_lightCullingNumThreadGroups.x * m_lightCullingNumThreadGroups.y * l_averangeOverlapLight;

	m_lightIndexListGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("LightIndexListGBDC/");
	m_lightIndexListGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightIndexListGBDC->m_ElementCount = l_elementCount;
	m_lightIndexListGBDC->m_ElementSize = sizeof(uint32_t);

	return true;
}

bool LightCullingPass::createLightGridTDC()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_lightGridTDC = g_Engine->getRenderingServer()->AddTextureDataComponent("LightGrid/");
	m_lightGridTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_lightGridTDC->m_TextureDesc.Width = m_lightCullingNumThreadGroups.x;
	m_lightGridTDC->m_TextureDesc.Height = m_lightCullingNumThreadGroups.y;
	m_lightGridTDC->m_TextureDesc.Usage = TextureUsage::Sample;
	m_lightGridTDC->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGridTDC->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;

	return true;
}

bool LightCullingPass::createHeatMapTDC()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_heatMapTDC = g_Engine->getRenderingServer()->AddTextureDataComponent("LightCullingDebug/");
	m_heatMapTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_heatMapTDC->m_TextureDesc.Usage = TextureUsage::Sample;

	return true;
}

bool LightCullingPass::Setup()
{
	m_SPC_TileFrustum = g_Engine->getRenderingServer()->AddShaderProgramComponent("TileFrustum/");
	m_SPC_TileFrustum->m_ShaderFilePaths.m_CSPath = "tileFrustum.comp/";

	m_SPC_LightCulling = g_Engine->getRenderingServer()->AddShaderProgramComponent("LightCulling/");
	m_SPC_LightCulling->m_ShaderFilePaths.m_CSPath = "lightCulling.comp/";

	////
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_RPDC_TileFrustum = g_Engine->getRenderingServer()->AddRenderPassDataComponent("ComputePass_TileFrustum/");
	m_RPDC_TileFrustum->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs.resize(3);
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 6;

	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RPDC_TileFrustum->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;

	m_RPDC_TileFrustum->m_ShaderProgram = m_SPC_TileFrustum;

	////
	m_RPDC_LightCulling = g_Engine->getRenderingServer()->AddRenderPassDataComponent("ComputePass_LightCulling/");
	m_RPDC_LightCulling->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs.resize(10);
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 3;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 6;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 2;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 3;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 4;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 0;
	m_RPDC_LightCulling->m_ResourceBinderLayoutDescs[9].m_IndirectBinding = true;

	m_RPDC_LightCulling->m_ShaderProgram = m_SPC_LightCulling;

	m_SDC_LightCulling = g_Engine->getRenderingServer()->AddSamplerDataComponent("ComputePass_LightCulling/");

	////
	createGridFrustumsBuffer();
	createLightIndexCounterBuffer();
	createLightIndexListBuffer();
	createLightGridTDC();
	createHeatMapTDC();

	return true;
}

bool LightCullingPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_tileFrustumGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_lightListIndexCounterGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_lightIndexListGBDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_lightGridTDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_heatMapTDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_TileFrustum);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_LightCulling);

	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC_LightCulling);

	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_TileFrustum);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_LightCulling);

	return true;
}

bool LightCullingPass::PrepareCommandList()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_lightListIndexCounter = 1;
	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_lightListIndexCounterGBDC, &l_lightListIndexCounter);

	DispatchParamsConstantBuffer l_tileFrustumWorkload;
	l_tileFrustumWorkload.numThreadGroups = m_tileFrustumNumThreadGroups;
	l_tileFrustumWorkload.numThreads = m_tileFrustumNumThreads;

	DispatchParamsConstantBuffer lightCullingWorkload;
	lightCullingWorkload.numThreadGroups = m_lightCullingNumThreadGroups;
	lightCullingWorkload.numThreads = m_lightCullingNumThreads;

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_tileFrustumWorkload, 0, 1);
	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &lightCullingWorkload, 1, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC_TileFrustum, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_TileFrustum);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC_TileFrustum);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_TileFrustum, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_TileFrustum, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 1, 6, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_TileFrustum, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite, 0);

	g_Engine->getRenderingServer()->DispatchCompute(m_RPDC_TileFrustum, m_tileFrustumNumThreadGroups.x, m_tileFrustumNumThreadGroups.y, m_tileFrustumNumThreadGroups.z);

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_TileFrustum, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite, 0);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC_TileFrustum);

	////
	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC_LightCulling, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_LightCulling);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC_LightCulling);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_SDC_LightCulling->m_ResourceBinder, 9, 0);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_PointLightGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 2, 6, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightListIndexCounterGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightIndexListGBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightGridTDC->m_ResourceBinder, 6, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_heatMapTDC->m_ResourceBinder, 7, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, OpaquePass::GetRPDC()->m_DepthStencilRenderTarget->m_ResourceBinder, 8, 0, Accessibility::ReadOnly);

	// @TODO: Buggy on OpenGL + Nvidia
	g_Engine->getRenderingServer()->DispatchCompute(m_RPDC_LightCulling, m_lightCullingNumThreadGroups.x, m_lightCullingNumThreadGroups.y, m_lightCullingNumThreadGroups.z);

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_tileFrustumGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightListIndexCounterGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightIndexListGBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_lightGridTDC->m_ResourceBinder, 6, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, m_heatMapTDC->m_ResourceBinder, 7, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC_LightCulling, ShaderStage::Compute, OpaquePass::GetRPDC()->m_DepthStencilRenderTarget->m_ResourceBinder, 8, 0, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC_LightCulling);

	return true;
}

bool LightCullingPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_TileFrustum);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_LightCulling);

	return true;
}

RenderPassDataComponent* LightCullingPass::GetTileFrustumRPDC()
{
	return m_RPDC_TileFrustum;
}

RenderPassDataComponent* LightCullingPass::GetLightCullingRPDC()
{
	return m_RPDC_LightCulling;
}

IResourceBinder* LightCullingPass::GetLightGrid()
{
	return m_lightGridTDC->m_ResourceBinder;
}

IResourceBinder* LightCullingPass::GetLightIndexList()
{
	return m_lightIndexListGBDC->m_ResourceBinder;
}

IResourceBinder* LightCullingPass::GetHeatMap()
{
	return m_heatMapTDC->m_ResourceBinder;
}