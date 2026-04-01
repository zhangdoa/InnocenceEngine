#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Component/GPUBufferComponent.h"

namespace Inno
{
	class GPUPathTracerPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GPUPathTracerPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();
		CommandListComponent* GetToneMapCommandList();

		void ResetAccumulation();

	private:
		struct GPUPathTracerVertex
		{
			float posX, posY, posZ;
			float normX, normY, normZ;
		};

		struct MeshOffsetData
		{
			uint32_t m_VertexOffset;
			uint32_t m_IndexOffset;
			uint32_t m_VertexCount;
			uint32_t m_IndexCount;
		};

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		// DXR ray tracing pass
		RenderPassComponent*    m_RayTracingRenderPassComp = nullptr;
		ShaderProgramComponent* m_RayTracingSPC            = nullptr;

		// Tonemap compute pass
		RenderPassComponent*    m_ToneMapRenderPassComp = nullptr;
		ShaderProgramComponent* m_ToneMapSPC            = nullptr;
		CommandListComponent*   m_ToneMapCommandList    = nullptr;

		// Owned GPU resources
		TextureComponent*   m_AccumulationBuffer = nullptr;
		TextureComponent*   m_ToneMapOutput      = nullptr;
		GPUBufferComponent* m_FrameCountCB       = nullptr;

		// Geometry mega-buffers (rebuilt on scene load)
		GPUBufferComponent* m_MegaVertexBuffer = nullptr;
		GPUBufferComponent* m_MegaIndexBuffer  = nullptr;
		GPUBufferComponent* m_MeshOffsetBuffer = nullptr;

		// Camera movement detection
		Math::Mat4 m_PrevViewMatrix = {};
		uint32_t m_FrameCount     = 1;

		// Scene callbacks
		std::function<void()> f_sceneLoadedCallback;
		std::function<void()> f_sceneUnloadingCallback;

		ShaderStage m_ShaderStage = ShaderStage::Invalid;

		void RebuildGeometryBuffers();
	};
}
