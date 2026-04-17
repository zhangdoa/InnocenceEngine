#include "SunShadowCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool SunShadowCullingPass::Setup(IServiceConfig *systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SunShadowCullingPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "sunShadowCulling.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SunShadowCullingPass/");

	m_IndirectDrawCommandBuffer = g_Engine->Get<GPUBufferResourceService>()->Add("SunShadowCullingPass/IndirectDrawCommandBuffer/");
	m_IndirectDrawCommandBuffer->m_Usage = GPUBufferUsage::IndirectDraw;
	m_IndirectDrawCommandBuffer->m_ElementCount = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability().maxMeshes;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);

	// b0 - PerFrame constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// t0 - GPU model data buffer (input - StructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// t1 - Material buffer (input - StructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// u0 - Indirect draw command buffer (output - RWStructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SunShadowCullingPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowCullingPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_IndirectDrawCommandBuffer);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SunShadowCullingPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<GPUBufferResourceService>()->Delete(m_IndirectDrawCommandBuffer);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowCullingPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_IndirectDrawCommandBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_drawCallService = g_Engine->Get<DrawCallService>();
	auto& l_gpuModelData = l_drawCallService->GetGPUModelData();
	uint32_t l_modelCount = static_cast<uint32_t>(l_gpuModelData.size());
	if (l_modelCount == 0)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	// Bind resources for compute shader
	auto l_perFrameCBuffer = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_gpuModelDataBuffer = l_drawCallService->GetGPUModelDataBuffer();
	auto l_materialBuffer = l_drawCallService->GetMaterialBuffer();

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCBuffer, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_gpuModelDataBuffer, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_materialBuffer, 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_IndirectDrawCommandBuffer, 3);

	// Dispatch culling compute shader
	// Calculate thread groups based on model count
	uint32_t l_threadGroupSize = 64; // Must match THREAD_GROUP_SIZE in shader
	uint32_t l_threadGroups = (l_modelCount + l_threadGroupSize - 1) / l_threadGroupSize;
	
	// Ensure we dispatch at least 1 thread group
	l_threadGroups = std::max(l_threadGroups, 1u);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_threadGroups, 1, 1);

	// Update CPU-side state tracking to UAV so the graphics pass knows to issue
	// a UAV→INDIRECT_ARGUMENT barrier before ExecuteIndirect. The compute queue
	// cannot issue a barrier involving INDIRECT_ARGUMENT state, and fence sync
	// handles memory visibility — only the tracking needs updating.
	auto l_currentFrame = l_fmService->GetCurrentFrame();
	m_IndirectDrawCommandBuffer->SetCurrentState(l_currentFrame, m_IndirectDrawCommandBuffer->m_WriteState);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SunShadowCullingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowCullingPass::GetResult()
{
	return m_IndirectDrawCommandBuffer;
}