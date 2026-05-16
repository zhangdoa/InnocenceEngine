#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheReprojectionPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheReprojectionPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetCurrentFrameResult();
		TextureComponent* GetPreviousFrameResult();
		TextureComponent* GetCurrentProbePosition();
		TextureComponent* GetPreviousProbePosition();
		TextureComponent* GetCurrentProbeNormal();
		TextureComponent* GetPreviousProbeNormal();
		TextureComponent* GetProbeMask();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_RadianceCache_Odd;
		TextureComponent* m_RadianceCache_Even;
		TextureComponent* m_ProbePosition_Odd;
		TextureComponent* m_ProbePosition_Even;
		TextureComponent* m_ProbeNormal_Odd;
		TextureComponent* m_ProbeNormal_Even;
		TextureComponent* m_ProbeMask;
		// Paper §2.1.8 side cache: preserves the last-good radiance atlas
		// block + (pos, normal, frame) meta of each tile across
		// reprojection invalidations, so a tile briefly lost to occlusion
		// or fast motion can restore its radiance instead of snapping to
		// zero for N frames until the next RayGen spawn.
		TextureComponent* m_SideCache_Atlas;    // mirrors the radiance atlas layout; preserved on failed reprojection
		TextureComponent* m_SideCache_PosFrame; // pos.xyz in RGB, asfloat(frameIndex) in A
		TextureComponent* m_SideCache_Normal;   // normal.xyz in RGB

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();

	};
} // namespace Inno
