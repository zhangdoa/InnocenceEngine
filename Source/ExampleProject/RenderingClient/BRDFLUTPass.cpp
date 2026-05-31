#include "BRDFLUTPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RenderGraph/RenderGraphService.h"

using namespace Inno;

namespace
{
	// TASK-227 Phase-0 coexistence seam (RFC §10): when true, BRDFLUTPass is
	// driven by the data-declared render graph; consumers read GetResult() /
	// GetRenderPassComp() unchanged. The imperative path below is preserved
	// verbatim under the false branch, so the migration stays reversible and
	// visual parity is verifiable against it.
	constexpr bool g_UseRenderGraph = true;
	const char* const g_GraphFile = "ExampleProject/RenderGraph/ExampleRenderGraph.json";
}

bool BRDFLUTPass::SetupFromRenderGraph()
{
	auto l_graphService = g_Engine->Get<RenderGraphService>();
	if (!l_graphService->LoadGraph(g_GraphFile))
		return false;

	auto l_node = l_graphService->FindNode("BRDFLUTPass");
	if (!l_node)
	{
		Log(Error, "BRDFLUTPass: render graph has no BRDFLUTPass node.");
		return false;
	}

	m_ShaderProgramComp = l_node->m_ShaderProgram;
	m_RenderPassComp = l_node->m_RenderPass;
	m_Result = static_cast<TextureComponent*>(l_node->m_PrimaryOutput);
	m_CommandListComp_Compute = l_node->m_CommandList_Compute;
	m_CommandListComp_Graphics = l_node->m_CommandList_Graphics;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool BRDFLUTPass::Setup(IServiceConfig *systemConfig)
{
	if (g_UseRenderGraph)
		return SetupFromRenderGraph();

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("BRDFLUTPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("BRDFLUTPass");
	m_Result = g_Engine->Get<TextureResourceService>()->Add("BRDF LUT");
	m_Result->m_TextureDesc.Width = 512;
	m_Result->m_TextureDesc.Height = 512;
	m_Result->m_TextureDesc.DepthOrArraySize = 1;
	m_Result->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	m_Result->m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	m_Result->m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	m_Result->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;
	
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(1);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ComputeOnly;	
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("BRDFLUTPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("BRDFLUTPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool BRDFLUTPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);	
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (g_UseRenderGraph)
	{
		auto l_node = g_Engine->Get<RenderGraphService>()->FindNode("BRDFLUTPass");
		if (!g_Engine->Get<RenderGraphService>()->RecordNode(l_node))
			return false;

		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
    l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 0);
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, 32, 32, 1);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

RenderPassComponent *BRDFLUTPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *BRDFLUTPass::GetResult()
{
	return m_Result;
}