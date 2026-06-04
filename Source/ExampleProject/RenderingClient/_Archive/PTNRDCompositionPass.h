#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

namespace Inno
{
	// Unpacks NRD outputs + re-modulates by primary-hit albedo: writes
	// `albedo * outDiff + outSpec` into m_Result for the tonemap input.
	// HLSL: PTNRDComposition.comp.
	// Bindings (HLSL register block matched 1:1 in C++ resize):
	//   b0 (set 0, idx 0)  - PerFrameConstantBuffer
	//   t0 (set 1, idx 0)  - PT_RT0_PositionInstanceID  (sky test)
	//   t1 (set 1, idx 1)  - PT_RT2_AlbedoRoughness     (re-mod factor)
	//   t2 (set 1, idx 2)  - NRD_OutDiffRadianceHitDist
	//   t3 (set 1, idx 3)  - NRD_OutSpecRadianceHitDist
	//   t4 (set 1, idx 4)  - PT_AccumBuffer             (sky pass-through)
	//   u0 (set 2, idx 0)  - out_FinalRadiance          (tonemap input)
	class PTNRDCompositionPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTNRDCompositionPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// Output. Tonemap source under Inno::NRD::ENABLED.
		TextureComponent* GetResult() { return m_Result; }

	private:
		ObjectStatus            m_ObjectStatus      = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
		TextureComponent*       m_Result            = nullptr;

		bool RenderTargetsCreationFunc();
	};
}
