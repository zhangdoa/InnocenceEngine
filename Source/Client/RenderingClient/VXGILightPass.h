#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGILightPassRenderingContext: public IRenderingContext
	{
	public:
		GPUResourceComponent* m_AlbedoVolume;
		GPUResourceComponent* m_NormalVolume;
		uint32_t m_resolution;
	};

	class VXGILightPass: IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGILightPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetIlluminanceVolume();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		TextureComponent *m_IlluminanceVolume;
	};
} // namespace Inno
