#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

#include "NRDConstants.h"

namespace Inno
{
#if INNO_BUILD_WITH_NRD
	class NRDIntegrationAdapter;
#endif

	// Engine-side pass shell wrapping the NRD raw-D3D12 adapter (TASK-77.4
	// CL-3). Exposes the standard IRenderPass lifecycle (Setup / Initialize /
	// Update / Terminate / PrepareCommandList) so the rendering-client
	// dispatch loop treats it like any other compute pass; internally it
	// hands a raw ID3D12GraphicsCommandList* to NRDIntegrationAdapter and
	// records the NRD ReBLUR_DIFFUSE_SPECULAR dispatch sequence.
	//
	// Lifecycle pinning:
	//   - Setup: allocates the adapter object only (no D3D12 work).
	//   - Initialize: lazily called from the rendering client AFTER
	//     PTNRDFormatConvertPass::Initialize (so the engine's DX12 device is
	//     up). Forwards to NRDIntegrationAdapter::Initialize.
	//   - Terminate: calls NRDIntegrationAdapter::Terminate before the device
	//     dies, then drops the adapter object.
	//   - PrepareCommandList: per-frame entry. Reads PTNRDFormatConvertPass's
	//     5 outputs as inputs, hands the raw command list to the adapter.
	//
	// This pass owns no engine TextureComponent*s of its own — the adapter's
	// GetOutDiffRadianceHitDist / GetOutSpecRadianceHitDist return borrowed
	// shells that the composition pass binds directly. This pass only owns
	// CommandListComponent allocations + the lifecycle of the adapter.
	class PTNRDDenoisePass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTNRDDenoisePass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// Borrowed NRD outputs — owned by the adapter, exposed for the
		// composition pass to bind. nullptr until Initialize succeeds AND
		// at least one DispatchDenoise call has run (the pool indices for
		// OUT_DIFF / OUT_SPEC are learned at first dispatch).
		// Always present so the composition pass compiles in either NRD
		// toggle state; in the OFF build both stubs return nullptr.
		TextureComponent* GetOutDiffRadianceHitDist();
		TextureComponent* GetOutSpecRadianceHitDist();

	private:
		ObjectStatus            m_ObjectStatus      = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
#if INNO_BUILD_WITH_NRD
		NRDIntegrationAdapter*  m_Adapter           = nullptr;
		uint32_t                m_FrameIndex        = 0;
		Math::Mat4              m_PrevWorldToView   = {};
		Math::Mat4              m_PrevViewToClip    = {};
		bool                    m_HasPrevMatrices   = false;
#endif
	};
}
