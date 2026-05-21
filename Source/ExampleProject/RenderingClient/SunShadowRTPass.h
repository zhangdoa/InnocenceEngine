#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TextureComponent;

	// Per-pixel R8 sun-visibility (0.0 fully shadowed, 1.0 lit). One thread per screen pixel
	// traces a single cone-jittered shadow ray (~0.27° half-angle, the sun's apparent disc); TAA
	// accumulation across frames softens the penumbra.
	class SunShadowRTPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SunShadowRTPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent* m_RenderPassComp = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;

		TextureComponent* m_SunVisibility = nullptr;

		ShaderStage m_ShaderStage = ShaderStage::Invalid;

		bool RenderTargetsCreationFunc();
		void OnResize();
		void CreateVisibilityBuffer();
	};
} // namespace Inno
