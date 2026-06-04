#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

namespace Inno
{
#if INNO_BUILD_WITH_NRD
	class NRDIntegrationAdapter;
#endif

	// IRenderPass shell over NRDIntegrationAdapter. Hands a raw D3D12 command list to the
	// adapter, which records the ReBLUR_DIFFUSE_SPECULAR dispatch sequence. Owns no
	// TextureComponents — output shells are adapter-owned.
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

		// Adapter-owned; nullptr until Initialize succeeds AND ≥1 DispatchDenoise has run
		// (pool indices for OUT_DIFF / OUT_SPEC are learned at first dispatch).
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
