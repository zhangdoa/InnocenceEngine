#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Common/GPUDataStructure.h"

namespace Inno
{
	class BSDFTestPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(BSDFTestPass)

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

		std::vector<PerObjectConstantBuffer> m_meshConstantBuffer;
		std::vector<MaterialConstantBuffer> m_materialConstantBuffer;

		const size_t m_shpereCount = 10;
	};
} // namespace Inno
