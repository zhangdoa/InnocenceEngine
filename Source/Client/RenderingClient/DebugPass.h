#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Services/BVHService.h"

namespace Inno
{
	struct DebugPerObjectConstantBuffer
	{
		Math::Mat4 m;
		uint32_t materialID;
		uint32_t padding[15];
	};

	struct DebugMaterialConstantBuffer
	{
		Math::Vec4 color;
	};

	class DebugPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(DebugPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
        bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

        void InitializeResourceBindingLayoutDescs() override;
		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;

		bool AddBVHNode(const BVHNode& node);
		DebugPerObjectConstantBuffer AddAABB(const AABB& aabb);
		
		GPUBufferComponent* m_debugSphereMeshGPUBufferComp;
		GPUBufferComponent* m_debugCubeMeshGPUBufferComp;
		GPUBufferComponent* m_debugMaterialGPUBufferComp;
		GPUBufferComponent* m_debugCameraFrustumGPUBufferComp;

		std::vector<MeshComponent*> m_debugCameraFrustumMeshComps;

		const size_t m_maxDebugMeshes = 65536;
		const size_t m_maxDebugMaterial = 512;
		std::vector<DebugPerObjectConstantBuffer> m_debugSphereConstantBuffer;
		std::vector<DebugPerObjectConstantBuffer> m_debugCubeConstantBuffer;
		std::vector<DebugPerObjectConstantBuffer> m_debugCameraFrustumConstantBuffer;
		std::vector<DebugMaterialConstantBuffer> m_debugMaterialConstantBuffer;
	};
} // namespace Inno
