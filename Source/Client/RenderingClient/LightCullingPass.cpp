#include "LightCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "TiledFrustumGenerationPass.h"
#include "OpaquePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool LightCullingPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_initialIndexCount = 1;

	m_lightListIndexCounter = l_renderingServer->AddGPUBufferComponent("LightListIndexCounter/");
	m_lightListIndexCounter->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightListIndexCounter->m_ElementCount = 1;
	m_lightListIndexCounter->m_ElementSize = sizeof(uint32_t);
	m_lightListIndexCounter->m_InitialData = &l_initialIndexCount;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&LightCullingPass::RenderTargetsCreationFunc, this);

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("LightCullingPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "lightCulling.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("LightCullingPass/");
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(10);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - PointLightList
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	// b2 - DispatchParams
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	// u0 - FrustumList
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	// u1 - LightIndexCounter
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;

	// u2 - LightIndexList
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;

	// u3 - LightGrid
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;

	// u4 - HeatMap (Debug)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 4;

	// t0 - Depth from OpaquePass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;

	// s0 - Sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("LightCullingPass/");

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LightCullingPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

	l_renderingServer->InitializeGPUBufferComponent(m_lightListIndexCounter);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LightCullingPass::Update()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_lightListIndexCounter = 1;
	l_renderingServer->UploadGPUBufferComponent(m_lightListIndexCounter, &l_lightListIndexCounter);

	DispatchParamsConstantBuffer lightCullingWorkload;
	lightCullingWorkload.numThreadGroups = m_numThreadGroups;
	lightCullingWorkload.numThreads = m_numThreads;

	auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	l_renderingServer->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &lightCullingWorkload, 1, 1);

	return true;
}

bool LightCullingPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteGPUBufferComponent(m_lightListIndexCounter);
	l_renderingServer->DeleteGPUBufferComponent(m_lightIndexList);
	l_renderingServer->DeleteTextureComponent(m_lightGrid);
	l_renderingServer->DeleteTextureComponent(m_heatMap);

	l_renderingServer->DeleteSamplerComponent(m_SamplerComp);
	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);
	l_renderingServer->DeleteShaderProgramComponent(m_ShaderProgramComp);
	
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightCullingPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_lightListIndexCounter->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (m_lightIndexList->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (m_lightGrid->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (m_heatMap->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_currentFrame = l_renderingServer->GetCurrentFrame();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, TiledFrustumGenerationPass::Get().GetTiledFrustum(), 3);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightListIndexCounter, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightIndexList, 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_lightGrid, 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_heatMap, 7);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput, 8);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 9);

	l_renderingServer->Dispatch(m_RenderPassComp, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

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

bool LightCullingPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_lightIndexList)
		l_renderingServer->DeleteGPUBufferComponent(m_lightIndexList);
	if (m_lightGrid)
		l_renderingServer->DeleteTextureComponent(m_lightGrid);
	if (m_heatMap)
		l_renderingServer->DeleteTextureComponent(m_heatMap);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_averageOverlapLight = 64; // Estimated average number of lights overlapping a tile
	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);
	m_numThreads = TVec4<uint32_t>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_numThreadGroups.x * m_numThreadGroups.y * l_averageOverlapLight;

	m_lightIndexList = l_renderingServer->AddGPUBufferComponent("LightIndexList/");
	m_lightIndexList->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightIndexList->m_ElementCount = l_elementCount;
	m_lightIndexList->m_ElementSize = sizeof(uint32_t);

	m_lightGrid = l_renderingServer->AddTextureComponent("LightGrid/");
	m_lightGrid->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_lightGrid->m_TextureDesc.Width = m_numThreadGroups.x;
	m_lightGrid->m_TextureDesc.Height = m_numThreadGroups.y;
	m_lightGrid->m_TextureDesc.Usage = TextureUsage::ColorAttachment;
	m_lightGrid->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGrid->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;

	m_heatMap = l_renderingServer->AddTextureComponent("LightCullingHeatMap/");
	m_heatMap->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_heatMap->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->InitializeGPUBufferComponent(m_lightIndexList);
	l_renderingServer->InitializeTextureComponent(m_lightGrid);
	l_renderingServer->InitializeTextureComponent(m_heatMap);

	return true;
}
