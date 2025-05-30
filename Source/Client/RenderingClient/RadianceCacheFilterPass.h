#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheFilterPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheFilterPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetFilteredResult();

	private:
		const uint32_t TILE_SIZE = 8;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_FilteredRadianceCache;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
