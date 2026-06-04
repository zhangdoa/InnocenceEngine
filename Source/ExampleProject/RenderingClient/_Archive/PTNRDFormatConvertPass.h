#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"

namespace Inno
{
	// Reads PT primary-hit GBuffer + per-lobe radiance UAVs and packs them into the five
	// textures NRD ReBLUR_DiffuseSpecular consumes (IN_VIEWZ, IN_NORMAL_ROUGHNESS, IN_MV,
	// IN_DIFF_RADIANCE_HITDIST, IN_SPEC_RADIANCE_HITDIST). HLSL: PTNRDFormatConvert.comp.
	class PTNRDFormatConvertPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(PTNRDFormatConvertPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// Pass-owned; nullptr when Inno::NRD::ENABLED is false.
		TextureComponent* GetNRDViewZ()                { return m_NRD_ViewZ; }
		TextureComponent* GetNRDNormalRoughness()      { return m_NRD_NormalRoughness; }
		TextureComponent* GetNRDMotionVector()         { return m_NRD_MotionVector; }
		TextureComponent* GetNRDDiffRadianceHitDist()  { return m_NRD_DiffRadianceHitDist; }
		TextureComponent* GetNRDSpecRadianceHitDist()  { return m_NRD_SpecRadianceHitDist; }

	private:
		ObjectStatus            m_ObjectStatus      = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;

		TextureComponent* m_NRD_ViewZ                = nullptr;
		TextureComponent* m_NRD_NormalRoughness      = nullptr;
		TextureComponent* m_NRD_MotionVector         = nullptr;
		TextureComponent* m_NRD_DiffRadianceHitDist  = nullptr;
		TextureComponent* m_NRD_SpecRadianceHitDist  = nullptr;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
