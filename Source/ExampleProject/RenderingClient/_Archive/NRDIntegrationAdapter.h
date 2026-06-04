// Drives NRD ReBLUR_DIFFUSE_SPECULAR via NRD's low-level Instance API + raw D3D12 — bypasses
// NRDIntegration.hpp + NRI. The engine's RenderPassComponent does not expose four binding-model
// features NRD requires (variable binding cardinality, CBV register-space override, static
// samplers at non-zero space, per-dispatch dynamic CB byte-offset); same dual-tier pattern as
// imgui_impl_dx12.cpp and DX12TextureResourceService_Mipmap.cpp.

#pragma once

#if INNO_BUILD_WITH_NRD

#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Component/CommandListComponent.h"
#include "../../Engine/Common/Math.h"

#include <NRD.h>

#include <cstdint>

namespace Inno
{
	struct NRDIntegrationAdapterImpl;

	// Borrowed-output TextureComponent shells from GetOutDiffRadianceHitDist /
	// GetOutSpecRadianceHitDist wrap adapter-owned D3D12 resources — engine consumers must NEVER
	// call TextureResourceService::Delete on them.
	struct NRDInputs
	{
		TextureComponent* m_ViewZ                = nullptr;
		TextureComponent* m_NormalRoughness      = nullptr;
		TextureComponent* m_MotionVector         = nullptr;
		TextureComponent* m_DiffRadianceHitDist  = nullptr;
		TextureComponent* m_SpecRadianceHitDist  = nullptr;

		Math::Mat4 m_WorldToView                 = {};
		Math::Mat4 m_ViewToClip                  = {};
		Math::Mat4 m_WorldToViewPrev             = {};
		Math::Mat4 m_ViewToClipPrev              = {};
		uint32_t   m_FrameIndex                  = 0;
		uint16_t   m_ResolutionX                 = 0;
		uint16_t   m_ResolutionY                 = 0;
		bool       m_ResetAccumulation           = false;
	};

	class NRDIntegrationAdapter
	{
	public:
		NRDIntegrationAdapter();
		~NRDIntegrationAdapter();

		NRDIntegrationAdapter(const NRDIntegrationAdapter&)            = delete;
		NRDIntegrationAdapter& operator=(const NRDIntegrationAdapter&) = delete;

		bool Initialize(uint16_t in_ResolutionX, uint16_t in_ResolutionY);

		// Idempotent; safe to call when Initialize never succeeded.
		void Terminate();

		// Caller hands inputs already in SHADER_RESOURCE state; outputs are left in
		// SHADER_RESOURCE for the composition pass on the same queue.
		bool DispatchDenoise(CommandListComponent* in_CommandList, const NRDInputs& in_Inputs);

		TextureComponent* GetOutDiffRadianceHitDist() const;
		TextureComponent* GetOutSpecRadianceHitDist() const;

		bool IsInitialized() const;

	private:
		NRDIntegrationAdapterImpl* m_Impl = nullptr;
	};
}

#endif // INNO_BUILD_WITH_NRD
