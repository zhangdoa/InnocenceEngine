// NRD ReBLUR integration via two layers of techdebt — both deliberate.
//
// Layer 1: We bypass NRDIntegration.hpp + NRI (NVIDIA's supported entry
// point) and call NRD's lower-level Instance API directly. NRDIntegration.
// hpp + NRI would pull a second NV third-party SDK and force engine-wide
// barrier-handoff invariant rework; we keep both costs out.
//
// Layer 2: NRD's Instance API requires four engine-binding-model features
// the engine's RenderPassComponent doesn't expose:
//   1. per-pipeline variable binding cardinality
//   2. CBV register-space override (NRD hard-codes space1)
//   3. static samplers at non-zero space
//   4. per-dispatch dynamic CB byte-offset
// Rather than refactor the engine binding model (option b1, engine-wide
// invariant work), we use a raw-D3D12 passthrough below RenderPassComponent
// — the same dual-tier pattern imgui_impl_dx12.cpp and
// DX12TextureResourceService_Mipmap.cpp already use as private subsystems.
//
// Trade-off: NRD release updates may require parallel rewrites of this
// dispatch sequence. Mitigated by SHA pin (2784717 in .gitmodules; updates
// are deliberate, not passive).
//
// Revisit when: a second third-party shader tree (FidelityFX, Streamline)
// lands and parallel-rewrite cost crosses (b1) engine-refactor cost; OR
// NRD rewires its Instance API across versions; OR engine grows a unified
// binding-model abstraction that absorbs the four gaps above.
//
// See TASK-77.4 Implementation Notes / "CL-3 approach pivot" + "CL-3
// sub-pivot" for the full decision context.

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

	// Adapter that drives NRD ReBLUR_DIFFUSE_SPECULAR via raw D3D12.
	// Lifetime contract:
	//   - Initialize() called after the engine's DX12 device is up; allocates
	//     the NRD Instance, root signatures, PSOs, permanent + transient
	//     texture pool, ring-buffered constant buffer, and the borrowed-output
	//     TextureComponent shells.
	//   - DispatchDenoise() called every frame on the engine's compute queue
	//     after PTNRDFormatConvertPass has packed inputs. The adapter records
	//     all NRD compute dispatches into the supplied raw D3D12 command list.
	//   - Terminate() called before the device dies. Drops all owned D3D12
	//     resources and invalidates the borrowed-output TextureComponent
	//     shells; the engine's binding cache must drop references to them
	//     before this point.
	//
	// Borrowed-output ownership shape (single new pattern this CL introduces):
	//   GetOutDiffRadianceHitDist() / GetOutSpecRadianceHitDist() return
	//   TextureComponent* whose m_GPUResources[0] is a raw ID3D12Resource*
	//   owned by the adapter (allocated as part of the NRD permanent pool).
	//   Their m_ReadHandles[0] is an SRV descriptor allocated on the engine's
	//   shader-visible heap; that SRV references the same D3D12 resource.
	//   The engine's normal BindGPUResource path treats them as ordinary
	//   compute-only textures, but TextureResourceService::Delete must NEVER
	//   be called on them — the adapter owns the underlying resource.
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

		// Lazy init — called from PTNRDDenoisePass::Initialize after the DX12
		// device is up. Returns false if NRD instance creation fails or any
		// PSO/root-signature/resource allocation fails. Failures are logged
		// loudly and m_Initialized stays false; subsequent DispatchDenoise
		// calls early-return.
		bool Initialize(uint16_t in_ResolutionX, uint16_t in_ResolutionY);

		// Called from PTNRDDenoisePass::Terminate before the device dies.
		// Idempotent; safe to call when Initialize never succeeded.
		void Terminate();

		// Per-frame dispatch — caller hands a raw graphics command list
		// (compute queue) plus the engine-side inputs already in the
		// SHADER_RESOURCE state. The adapter records all NRD compute
		// dispatches into the command list and leaves its own outputs in the
		// SHADER_RESOURCE state for the composition pass to read on the same
		// queue. Returns false on contract violation (uninitialized,
		// nullptr inputs, frame index regression).
		bool DispatchDenoise(CommandListComponent* in_CommandList, const NRDInputs& in_Inputs);

		// Borrowed outputs exposed as engine TextureComponent shells. Lifetime
		// is bound to this adapter — do not delete and do not survive the
		// adapter's Terminate. nullptr until Initialize succeeds.
		TextureComponent* GetOutDiffRadianceHitDist() const;
		TextureComponent* GetOutSpecRadianceHitDist() const;

		bool IsInitialized() const;

	private:
		NRDIntegrationAdapterImpl* m_Impl = nullptr;
	};
}

#endif // INNO_BUILD_WITH_NRD
