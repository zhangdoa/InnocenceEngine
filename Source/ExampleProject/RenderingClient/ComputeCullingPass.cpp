#include "ComputeCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

namespace
{
	// When true, a ComputeCulling subclass is graph-driven (binding table + dynamic
	// dispatch via the ComputeCulling kernel); a subclass with no matching node uses
	// the imperative path below. The indirect buffer stays imperative (its element
	// count is a runtime config value, maxMeshes) and is imported into the graph by name.
	constexpr bool g_UseRenderGraph = true;
}

bool ComputeCullingPass::SetupFromRenderGraph()
{
	const char* l_passName = GetPassName();
	const std::string l_bufferName = std::string(l_passName) + "/IndirectDrawCommandBuffer";

	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode(l_passName);
	if (!l_node)
		return false;

	// Indirect buffer stays imperative (runtime-config-sized); graph imports it.
	m_IndirectDrawCommandBuffer = g_Engine->Get<GPUBufferResourceService>()->Add(l_bufferName.c_str());
	m_IndirectDrawCommandBuffer->m_Usage = GPUBufferUsage::IndirectDraw;
	m_IndirectDrawCommandBuffer->m_ElementCount = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability().maxMeshes;

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool ComputeCullingPass::Setup(IServiceConfig* systemConfig)
{
	if (g_UseRenderGraph && SetupFromRenderGraph())
		return true;

	const char* l_passName = GetPassName();
	const std::string l_bufferName = std::string(l_passName) + "/IndirectDrawCommandBuffer";
	const std::string l_commandListName = std::string(l_passName) + "/Compute";

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add(l_passName);
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = GetComputeShaderPath();

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add(l_passName);

	m_IndirectDrawCommandBuffer = g_Engine->Get<GPUBufferResourceService>()->Add(l_bufferName.c_str());
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

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add(l_commandListName.c_str());
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool ComputeCullingPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_IndirectDrawCommandBuffer);

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool ComputeCullingPass::Terminate()
{
	g_Engine->Get<GPUBufferResourceService>()->Delete(m_IndirectDrawCommandBuffer);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus ComputeCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool ComputeCullingPass::PrepareCommandList(IRenderingContext* /*renderingContext*/)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_IndirectDrawCommandBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (g_UseRenderGraph)
	{
		// The ComputeCulling kernel owns the dynamic dispatch size, the empty-
		// model-set early-out, and the post-dispatch UAV state-tracker side effect.
		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode(GetPassName());
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	auto l_drawCallService = g_Engine->Get<DrawCallService>();
	auto& l_gpuModelData = l_drawCallService->GetGPUModelData();
	uint32_t l_modelCount = static_cast<uint32_t>(l_gpuModelData.size());
	if (l_modelCount == 0)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	auto l_perFrameCBuffer    = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_gpuModelDataBuffer = l_drawCallService->GetGPUModelDataBuffer();
	auto l_materialBuffer     = l_drawCallService->GetMaterialBuffer();

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCBuffer,          0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_gpuModelDataBuffer,       1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_materialBuffer,           2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_IndirectDrawCommandBuffer, 3);

	// Must match THREAD_GROUP_SIZE in the .comp shader.
	constexpr uint32_t kThreadGroupSize = 64;
	uint32_t l_threadGroups = (l_modelCount + kThreadGroupSize - 1) / kThreadGroupSize;
	l_threadGroups = l_threadGroups > 0 ? l_threadGroups : 1;

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_threadGroups, 1, 1);

	// CPU-side state tracking → UAV so the downstream graphics pass emits
	// a UAV→INDIRECT_ARGUMENT barrier before ExecuteIndirect. The compute
	// queue itself can't emit that transition; fence sync covers memory
	// visibility, we only need to update the tracker.
	auto l_currentFrame = l_fmService->GetCurrentFrame();
	m_IndirectDrawCommandBuffer->SetCurrentState(l_currentFrame, m_IndirectDrawCommandBuffer->m_WriteState);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* ComputeCullingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* ComputeCullingPass::GetResult()
{
	return m_IndirectDrawCommandBuffer;
}
