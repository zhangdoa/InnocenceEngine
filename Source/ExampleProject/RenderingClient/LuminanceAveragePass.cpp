#include "LuminanceAveragePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "LuminanceHistogramPass.h"

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
	// TASK-227.2 coexistence seam (RFC §10, mirrors BRDFLUTMSPass): when true,
	// LuminanceAveragePass is driven by the data-declared render graph. The graph
	// file is already loaded by BRDFLUTPass::Setup (runs earlier), so this pass
	// only adopts its node. The imperative path is preserved verbatim under the
	// false branch, so the migration stays reversible and parity-verifiable.
	constexpr bool g_UseRenderGraph = true;
}

bool LuminanceAveragePass::SetupFromRenderGraph()
{
	auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("LuminanceAveragePass");
	if (!l_node)
	{
		Log(Error, "LuminanceAveragePass: render graph has no LuminanceAveragePass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_luminanceAverage = static_cast<GPUBufferComponent*>(l_node->m_PrimaryOutput);
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LuminanceAveragePass::Setup(IServiceConfig* systemConfig)
{
	if (g_UseRenderGraph)
		return SetupFromRenderGraph();

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_Resizable = false;

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("LuminanceAveragePass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "luminanceAveragePass.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("LuminanceAveragePass");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// u0 - LuminanceHistogram
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	// u1 - LuminanceAverage
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("LuminanceAveragePass");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_luminanceAverage = g_Engine->Get<GPUBufferResourceService>()->Add("LuminanceAverageGPUBuffer");
	m_luminanceAverage->m_CPUAccessibility = Accessibility::Immutable;
	m_luminanceAverage->m_GPUAccessibility = Accessibility::ReadWrite;
	m_luminanceAverage->m_ElementCount = m_MaxResultToKeep;
	m_luminanceAverage->m_ElementSize = sizeof(float);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LuminanceAveragePass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	// Graph-driven path adopts components the RenderGraphService Added (resource +
	// render pass + command list); the graph does not Initialize them, so the
	// adopted pointers go through the same Initialize calls as the imperative path.
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_luminanceAverage);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LuminanceAveragePass::Update()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LuminanceAveragePass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<GPUBufferResourceService>()->Delete(m_luminanceAverage);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceAveragePass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceAveragePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (LuminanceHistogramPass::Get().GetResult()->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_luminanceAverage->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (g_UseRenderGraph)
	{
		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("LuminanceAveragePass");
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, LuminanceHistogramPass::Get().GetResult(), 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_luminanceAverage, 2);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, 1, 1, 1);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* LuminanceAveragePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LuminanceAveragePass::GetResult()
{
	return m_luminanceAverage;
}