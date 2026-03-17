#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGIConvertPassRenderingContext: public IRenderingContext
	{
	public:
		GPUResourceComponent* m_input;
		uint32_t m_resolution;
	};

	class VXGIConvertPass: public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIConvertPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetAlbedoVolume();
		GPUResourceComponent *GetNormalVolume();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		TextureComponent *m_AlbedoVolume;
		TextureComponent *m_NormalVolume;
	};
} // namespace Inno
