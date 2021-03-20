#pragma once
#include "../../Engine/Interface/IRenderingClient.h"
#include "../../Engine/Component/GPUResourceComponent.h"
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace Inno
{
	class VXGIRenderingConfig : public IRenderingConfig
	{
		public:
		uint32_t m_voxelizationResolution = 128;
		uint32_t m_numCones = 16;
		uint32_t m_coneTracingStep = 2;
		float m_coneTracingMaxDistance = 128.0f;
		uint32_t m_maxRay = 8;
		uint32_t m_maxProbe = 8;
		bool m_visualize = false;
		uint32_t m_multiBounceCount = 0;
		bool m_screenFeedback = false;
	};

	class VXGIRenderer : public IRenderingClient
	{
	public:
		INNO_CLASS_SINGLETON(VXGIRenderer);
		
		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Render(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const VXGIRenderingConfig& GetVXGIRenderingConfig();
		GPUResourceComponent *GetVoxelizationCBuffer();
		GPUResourceComponent *GetResult();
		GPUResourceComponent *GetVisualizationResult();
		
	private:
		ObjectStatus m_ObjectStatus;
		
		VXGIRenderingConfig m_VXGIRenderingConfig = VXGIRenderingConfig{};
		GPUBufferDataComponent *m_VXGICBuffer;

		GPUResourceComponent *m_result;

		bool m_isInitialLoadScene = true;
		std::function<void()> f_sceneLoadingFinishCallback;	
	};
}
