#pragma once
#include "../../Engine/Interface/IRenderingClient.h"
#include "../../Engine/Component/GPUResourceComponent.h"
#include "../../Engine/Component/GPUBufferComponent.h"

namespace Inno
{
	class VXGIRenderingConfig: public IRenderingConfig
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

	class VXGIRendererSystemConfig: public ISystemConfig
	{
	public:
		VXGIRenderingConfig m_VXGIRenderingConfig = {};
	};

	class VXGIRenderer : public IRenderingClient
	{
	public:
		INNO_CLASS_SINGLETON(VXGIRenderer);
		
		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUResourceComponent *GetVoxelizationCBuffer();
		GPUResourceComponent *GetResult();
		GPUResourceComponent *GetVisualizationResult();
		
	private:
		ObjectStatus m_ObjectStatus;
		
		GPUBufferComponent *m_VXGICBuffer;
		GPUResourceComponent *m_result;

		bool m_isInitialLoadScene = true;
		std::function<void()> f_sceneLoadingFinishedCallback;	
	};
}
