#include "GPUPathTracerHashGridFilterPass.h"
#include "HashGridCacheConstants.h"

#include "GPUPathTracerPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool GPUPathTracerHashGridFilterPass::Setup(IServiceConfig* systemConfig)
{
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("GPUPathTracerHashGridFilterPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "GPUPathTracerHashGridFilter.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("GPUPathTracerHashGridFilterPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger  = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// Binding layout: b0=FrameCountCB, u0=HashGridKeys, u1=HashGridScratch,
	// u2=HashGridValue. All three hash buffers stay in their persistent
	// ReadWrite state — same convention as the PT raygen + denoise passes.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);

	// b0 - FrameCountCB (set 0, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// u0 - HashGridKeys (set 1, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// u1 - HashGridScratch
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// u2 - HashGridValue
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerHashGridFilterPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool GPUPathTracerHashGridFilterPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool GPUPathTracerHashGridFilterPass::Terminate()
{
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus GPUPathTracerHashGridFilterPass::GetStatus()
{
	return m_ObjectStatus;
}

bool GPUPathTracerHashGridFilterPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto& l_pt = GPUPathTracerPass::Get();
	if (l_pt.GetStatus() != ObjectStatus::Activated)
		return false;

	auto* l_frameCount = l_pt.GetFrameCountCB();
	auto* l_keys       = l_pt.GetHashGridKeys();
	auto* l_scratch    = l_pt.GetHashGridScratch();
	auto* l_value      = l_pt.GetHashGridValue();
	if (!l_frameCount || l_frameCount->m_ObjectStatus != ObjectStatus::Activated) return false;
	if (!l_keys       || l_keys->m_ObjectStatus       != ObjectStatus::Activated) return false;
	if (!l_scratch    || l_scratch->m_ObjectStatus    != ObjectStatus::Activated) return false;
	if (!l_value      || l_value->m_ObjectStatus      != ObjectStatus::Activated) return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	// Compute-only — the three hash-grid buffers stay in persistent ReadWrite
	// across PT, this pass, and denoise, so no graphics-side resource
	// transition is needed. Mirrors the OpaqueCullingPass dispatch shape:
	// upstream WaitIfActive on the producer's compute Signal, single compute
	// CL, downstream Signal.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_frameCount, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_keys,       1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_scratch,    2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_value,      3);

	// One thread per slot, [numthreads(64,1,1)]. CELL_COUNT (2^20) is
	// divisible by 64 so no tail group; the kernel still bounds-checks for
	// safety in case CELL_COUNT changes later.
	const uint32_t l_groups = (HashGridCache::CELL_COUNT + 63u) / 64u;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groups, 1u, 1u);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* GPUPathTracerHashGridFilterPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}
