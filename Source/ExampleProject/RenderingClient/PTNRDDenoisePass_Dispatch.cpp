#include "PTNRDDenoisePass.h"

#include "NRDConstants.h"

#if INNO_BUILD_WITH_NRD
#include "NRDIntegrationAdapter.h"
#endif

#include "PTNRDFormatConvertPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Common/LogService.h"

#include <cstring>

using namespace Inno;

bool PTNRDDenoisePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

#if INNO_BUILD_WITH_NRD
	if (!m_Adapter)
		return false;
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto* l_format = &PTNRDFormatConvertPass::Get();
	if (l_format->GetStatus() != ObjectStatus::Activated)
		return false;

	NRDInputs l_inputs;
	l_inputs.m_ViewZ                = l_format->GetNRDViewZ();
	l_inputs.m_NormalRoughness      = l_format->GetNRDNormalRoughness();
	l_inputs.m_MotionVector         = l_format->GetNRDMotionVector();
	l_inputs.m_DiffRadianceHitDist  = l_format->GetNRDDiffRadianceHitDist();
	l_inputs.m_SpecRadianceHitDist  = l_format->GetNRDSpecRadianceHitDist();
	if (!l_inputs.m_ViewZ || !l_inputs.m_NormalRoughness || !l_inputs.m_MotionVector ||
	    !l_inputs.m_DiffRadianceHitDist || !l_inputs.m_SpecRadianceHitDist)
		return false;

	auto  l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto& l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer();

	// Lazy adapter init at first PrepareCommandList — engine resolution
	// stable, format-convert outputs allocated, DX12 device fully up.
	if (!m_Adapter->IsInitialized())
	{
		if (!m_Adapter->Initialize(static_cast<uint16_t>(l_resolution.x),
		                           static_cast<uint16_t>(l_resolution.y)))
		{
			Log(Error, "PTNRDDenoisePass: adapter init failed; pass will skip every frame.");
			return false;
		}
	}

	// Camera-jump detection mirrors GPUPathTracerPass::Update — view matrix
	// changed means reset accumulation.
	const bool l_resetAccum = !m_HasPrevMatrices ||
	                          std::memcmp(&l_perFrameCB.v, &m_PrevWorldToView, sizeof(Math::Mat4)) != 0;

	std::memcpy(&l_inputs.m_WorldToView,     &l_perFrameCB.v,           sizeof(Math::Mat4));
	std::memcpy(&l_inputs.m_ViewToClip,      &l_perFrameCB.p_original,  sizeof(Math::Mat4));
	std::memcpy(&l_inputs.m_WorldToViewPrev, &m_PrevWorldToView,        sizeof(Math::Mat4));
	std::memcpy(&l_inputs.m_ViewToClipPrev,  &m_PrevViewToClip,         sizeof(Math::Mat4));
	l_inputs.m_FrameIndex          = m_FrameIndex++;
	l_inputs.m_ResolutionX         = static_cast<uint16_t>(l_resolution.x);
	l_inputs.m_ResolutionY         = static_cast<uint16_t>(l_resolution.y);
	l_inputs.m_ResetAccumulation   = l_resetAccum;

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	// Compute CL: hand off to adapter. Adapter records all NRD compute
	// dispatches (root sig + PSO bind + descriptor table copy + CB upload +
	// dispatch) into the raw command list. ExecuteCommands wraps a
	// CommandListBegin/End around this; the adapter never touches the
	// engine's frame-management state tracker for non-borrowed resources,
	// so the engine's per-resource state remains correct after this pass.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	if (!m_Adapter->DispatchDenoise(m_CommandListComp_Compute, l_inputs))
	{
		l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);
		return false;
	}
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	std::memcpy(&m_PrevWorldToView, &l_perFrameCB.v,          sizeof(Math::Mat4));
	std::memcpy(&m_PrevViewToClip,  &l_perFrameCB.p_original, sizeof(Math::Mat4));
	m_HasPrevMatrices = true;

	return true;
#else
	return true;
#endif
}
