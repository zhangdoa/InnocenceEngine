#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	struct ProbePos
	{
		Math::Vec4 pos;
		Math::Vec4 index;
	};

	struct ProbeMeshData
	{
		Math::Mat4 m;
		Math::Vec4 index;
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
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;	
		
		GPUBufferComponent* m_probeSphereMeshGPUBufferComp = 0;
		std::vector<ProbeMeshData> m_probeSphereMeshData;
	};
} // namespace Inno
