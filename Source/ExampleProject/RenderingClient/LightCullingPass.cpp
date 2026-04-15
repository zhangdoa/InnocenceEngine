#include "LightCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/LightDataService.h"

#include "TiledFrustumGenerationPass.h"
#include "OpaquePass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool LightCullingPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_initialIndexCount = 1;

	m_lightListIndexCounter = g_Engine->Get<GPUBufferResourceService>()->Add("LightListIndexCounter/");
	m_lightListIndexCounter->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightListIndexCounter->m_ElementCount = 1;
	m_lightListIndexCounter->m_ElementSize = sizeof(uint32_t);
	m_lightListIndexCounter->m_InitialData = &l_initialIndexCount;

	m_DispatchParamsGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("LightCullingDispatchParams/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&LightCullingPass::RenderTargetsCreationFunc, this);

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("LightCullingPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "lightCulling.comp/";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("LightCullingPass/");
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

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("LightCullingPass/");

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("LightCullingPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("LightCullingPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LightCullingPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_DispatchParamsGPUBufferComp->m_ElementCount = 1;
	m_DispatchParamsGPUBufferComp->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
	m_DispatchParamsGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_DispatchParamsGPUBufferComp);
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_lightListIndexCounter);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LightCullingPass::Update()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_lightListIndexCounter = 1;
	g_Engine->Get<GPUBufferResourceService>()->Upload(m_lightListIndexCounter, &l_lightListIndexCounter);

	DispatchParamsConstantBuffer lightCullingWorkload;
	lightCullingWorkload.numThreadGroups = m_numThreadGroups;
	lightCullingWorkload.numThreads = m_numThreads;

	g_Engine->Get<GPUBufferResourceService>()->Upload(m_DispatchParamsGPUBufferComp, &lightCullingWorkload, 0, 1);

	return true;
}

bool LightCullingPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<GPUBufferResourceService>()->Delete(m_lightListIndexCounter);
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_lightIndexList);
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_DispatchParamsGPUBufferComp);
	g_Engine->Get<TextureResourceService>()->Delete(m_lightGrid);
	g_Engine->Get<TextureResourceService>()->Delete(m_heatMap);

	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);
	
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

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_currentFrame = l_fmService->GetCurrentFrame();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_PointLightGPUBufferComp = g_Engine->Get<LightDataService>()->GetPointLightBuffer();

	// Use graphics command list to transition depth buffer from DEPTH_WRITE to NON_PIXEL_SHADER_RESOURCE state
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_DispatchParamsGPUBufferComp, 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, TiledFrustumGenerationPass::Get().GetTiledFrustum(), 3);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_lightListIndexCounter, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_lightIndexList, 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_lightGrid, 6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_heatMap, 7);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput, 8);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_SamplerComp, 9);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, m_numThreadGroups.x, m_numThreadGroups.y, m_numThreadGroups.z);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

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
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_lightIndexList)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_lightIndexList);
	if (m_lightGrid)
		g_Engine->Get<TextureResourceService>()->Delete(m_lightGrid);
	if (m_heatMap)
		g_Engine->Get<TextureResourceService>()->Delete(m_heatMap);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_averageOverlapLight = 64; // Estimated average number of lights overlapping a tile
	auto l_numThreadGroupsX = std::ceil(l_viewportSize.x / m_tileSize);
	auto l_numThreadGroupsY = std::ceil(l_viewportSize.y / m_tileSize);

	m_numThreadGroups = TVec4<uint32_t>((uint32_t)l_numThreadGroupsX, (uint32_t)l_numThreadGroupsY, 1, 0);
	m_numThreads = TVec4<uint32_t>(m_tileSize, m_tileSize, 1, 0);

	auto l_elementCount = m_numThreadGroups.x * m_numThreadGroups.y * l_averageOverlapLight;

	m_lightIndexList = g_Engine->Get<GPUBufferResourceService>()->Add("LightIndexList/");
	m_lightIndexList->m_GPUAccessibility = Accessibility::ReadWrite;
	m_lightIndexList->m_ElementCount = l_elementCount;
	m_lightIndexList->m_ElementSize = sizeof(uint32_t);

	m_lightGrid = g_Engine->Get<TextureResourceService>()->Add("LightGrid/");
	m_lightGrid->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_lightGrid->m_TextureDesc.Width = m_numThreadGroups.x;
	m_lightGrid->m_TextureDesc.Height = m_numThreadGroups.y;
	m_lightGrid->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	m_lightGrid->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RG;
	m_lightGrid->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;

	m_heatMap = g_Engine->Get<TextureResourceService>()->Add("LightCullingHeatMap/");
	m_heatMap->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_heatMap->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_lightIndexList);
	g_Engine->Get<TextureResourceService>()->Initialize(m_lightGrid);
	g_Engine->Get<TextureResourceService>()->Initialize(m_heatMap);

	return true;
}
