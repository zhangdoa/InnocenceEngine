#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGIConvertPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIConvertPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		GPUResourceComponent *GetLuminanceVolume();
		GPUResourceComponent *GetNormalVolume();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;
		TextureDataComponent *m_TDC;
		TextureDataComponent *m_luminanceVolume;
		TextureDataComponent *m_normalVolume;
	};
} // namespace Inno
