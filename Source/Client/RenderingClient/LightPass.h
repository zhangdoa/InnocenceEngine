#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class LightPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LightPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		TextureDataComponent *GetLuminanceResult();
		TextureDataComponent *GetIlluminanceResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;
		SamplerDataComponent *m_SDC;	
		TextureDataComponent *m_TDC_Luminance;
		TextureDataComponent *m_TDC_Illuminance;
	};
} // namespace Inno
