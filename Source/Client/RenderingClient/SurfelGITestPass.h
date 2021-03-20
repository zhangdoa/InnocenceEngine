#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	struct ProbePos
	{
		InnoMath::Vec4 pos;
		InnoMath::Vec4 index;
	};

	struct ProbeMeshData
	{
		InnoMath::Mat4 m;
		InnoMath::Vec4 index;
		float padding[12];
	};

	class SurfelGITestPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SurfelGITestPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;
		SamplerDataComponent *m_SDC;	
		TextureDataComponent* m_TDC;
		
		GPUBufferDataComponent* m_probeSphereMeshGBDC = 0;
		std::vector<ProbeMeshData> m_probeSphereMeshData;
	};
} // namespace Inno
