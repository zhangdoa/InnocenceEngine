#include "PTNRDDenoisePass.h"

#include "NRDConstants.h"

#if INNO_BUILD_WITH_NRD
#include "NRDIntegrationAdapter.h"
#endif

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"

using namespace Inno;

bool PTNRDDenoisePass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

#if INNO_BUILD_WITH_NRD
	// Render pass shell — kept minimal because the adapter records its own
	// dispatches against a raw ID3D12GraphicsCommandList*. The engine
	// RenderPassComponent here exists only to satisfy the dispatch-loop
	// status checks and own the per-pass CommandListComponent pair (the
	// dispatch-loop's ExecuteCommands gates on Activated + !IsBypassed).
	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTNRDDenoisePass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType     = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger   = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	// Empty binding-layout vector — adapter does its own binding via raw
	// D3D12 SetComputeRootSignature / SetComputeRootDescriptorTable. The
	// engine's BindGPUResource path is bypassed for this pass.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.clear();

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTNRDDenoisePass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;
	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTNRDDenoisePass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_Adapter = new NRDIntegrationAdapter();
#endif

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTNRDDenoisePass::Initialize()
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

#if INNO_BUILD_WITH_NRD
	// The render-pass-component path here is intentionally minimal — the
	// adapter owns its own root signatures and pipeline states (one per
	// NRD pipeline). The render-pass shell carries no shader; the engine
	// PSO-creation path early-outs on a null m_ShaderProgram (gate added
	// in DX12RenderPassResourceService_Pipeline.cpp), so Initialize here
	// only allocates semaphores + fence events — exactly what the
	// Execute / Signal / Wait chain needs in
	// ExampleRenderingClient_ExecuteCommands.cpp.
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;
#endif
	return true;
}

bool PTNRDDenoisePass::Update()
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

#if INNO_BUILD_WITH_NRD
	// Activation gate: render-pass-shell components ready AND adapter exists.
	// Per-frame initialization is lazy (first PrepareCommandList call); we
	// always advertise Activated once shells are in place, so the dispatch
	// loop calls PrepareCommandList where the lazy init runs.
	const bool l_ready = (m_RenderPassComp != nullptr) && (m_Adapter != nullptr) &&
	                     (m_RenderPassComp->m_ObjectStatus == ObjectStatus::Activated);
	m_ObjectStatus = l_ready ? ObjectStatus::Activated : ObjectStatus::Suspended;
#endif
	return true;
}

bool PTNRDDenoisePass::Terminate()
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

#if INNO_BUILD_WITH_NRD
	if (m_Adapter)
	{
		m_Adapter->Terminate();
		delete m_Adapter;
		m_Adapter = nullptr;
	}
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
#endif

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PTNRDDenoisePass::GetStatus()
{
	return m_ObjectStatus;
}

RenderPassComponent* PTNRDDenoisePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* PTNRDDenoisePass::GetOutDiffRadianceHitDist()
{
#if INNO_BUILD_WITH_NRD
	return m_Adapter ? m_Adapter->GetOutDiffRadianceHitDist() : nullptr;
#else
	return nullptr;
#endif
}

TextureComponent* PTNRDDenoisePass::GetOutSpecRadianceHitDist()
{
#if INNO_BUILD_WITH_NRD
	return m_Adapter ? m_Adapter->GetOutSpecRadianceHitDist() : nullptr;
#else
	return nullptr;
#endif
}
