#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheFilterVerticalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheFilterVerticalPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetResult();

	private:
		const uint32_t TILE_SIZE = 8;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno