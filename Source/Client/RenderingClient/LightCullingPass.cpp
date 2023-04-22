#include "LightCullingPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "TiledFrustumGenerationPass.h"
#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool LightCullingPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_initialIndexCount = 1;

	m_lightListIndexCounter = g_Engine->getRenderingServer()->AddGPUBufferComponent("LightListIndexCounter/");
	m_lightListIndexCounter->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightListIndexCounter->m_ElementCount = 1;
	m_lightListIndexCounter->m_ElementSize = sizeof(uint32_t);
	m_lightListIndexCounter->m_InitialData = &l_initialIndexCount;

	auto l_averangeOverlapLight = 64;

	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();

	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);
	m_numThreads = TVec4<uint32_t>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_numThreadGroups.x * m_numThreadGroups.y * l_averangeOverlapLight;

	m_lightIndexList = g_Engine->getRenderingServer()->AddGPUBufferComponent("LightIndexList/");
	m_lightIndexList->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightIndexList->m_ElementCount = l_elementCount;
	m_lightIndexList->m_ElementSize = sizeof(uint32_t);

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	
	m_lightGrid = g_Engine->getRenderingServer()->AddTextureComponent("LightGrid/");
	m_lightGrid->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_lightGrid->m_TextureDesc.Width = m_numThreadGroups.x;
	m_lightGrid->m_TextureDesc.Height = m_numThreadGroups.y;
	m_lightGrid->m_TextureDesc.Usage = TextureUsage::Sample;
	m_lightGrid->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGrid->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;

	m_heatMap = g_Engine->getRenderingServer()->AddTextureComponent("LightCullingHeatMap/");
	m_heatMap->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_heatMap->m_TextureDesc.Usage = TextureUsage::Sample;

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("LightCullingPass/");
	m_SPC->m_ShaderFilePaths.m_CSPath = "lightCulling.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("LightCullingPass/");
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(10);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 3;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 6;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("ComputePass/");

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LightCullingPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_lightListIndexCounter);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_lightIndexList);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_lightGrid);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_heatMap);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LightCullingPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightCullingPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_lightListIndexCounter = 1;
	g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_lightListIndexCounter, &l_lightListIndexCounter);

	DispatchParamsConstantBuffer lightCullingWorkload;
	lightCullingWorkload.numThreadGroups = m_numThreadGroups;
	lightCullingWorkload.numThreads = m_numThreads;

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &lightCullingWorkload, 1, 1);

	////
	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 9);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, TiledFrustumGenerationPass::Get().GetTiledFrustum(), 3, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightListIndexCounter, 4, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightIndexList, 5, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightGrid, 6, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_heatMap, 7, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget, 8, Accessibility::ReadOnly);

	// @TODO: Buggy on OpenGL + Nvidia
	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, TiledFrustumGenerationPass::Get().GetTiledFrustum(), 3, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightListIndexCounter, 4, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightIndexList, 5, Accessibility::ReadWrite, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightGrid, 6, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_heatMap, 7, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget, 8, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* LightCullingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LightCullingPass::GetLightGrid()
{
	return m_lightGrid;
}

GPUResourceComponent* LightCullingPass::GetLightIndexList()
{
	return m_lightIndexList;
}

GPUResourceComponent* LightCullingPass::GetHeatMap()
{
	return m_heatMap;
}