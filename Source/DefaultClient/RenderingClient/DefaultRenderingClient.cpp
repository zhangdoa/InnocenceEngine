#pragma once
#include "DefaultRenderingClient.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowCullingPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"
#include "SunShadowBlurEvenPass.h"
#include "OpaqueCullingPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheFilterHorizontalPass.h"
#include "RadianceCacheFilterVerticalPass.h"
#include "RadianceCacheIntegrationPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentGeometryProcessPass.h"
#include "TransparentBlendPass.h"
#include "VolumetricPass.h"
#include "VXGIRenderer.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/Services/HIDService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Common/Task.h"
#include "../../Engine/Common/TaskScheduler.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	class DefaultRenderingClientImpl : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultRenderingClientImpl);

		// Inherited via IRenderingClient
		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool PrepareCommands() override;
		bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		std::function<void()> f_showLightHeatmap;
		std::function<void()> f_showProbe;
		std::function<void()> f_showVoxel;
		std::function<void()> f_showTransparent;
		std::function<void()> f_showVolumetric;
		std::function<void()> f_saveScreenCapture;

		bool m_showLightHeatmap = false;
		bool m_showProbe = false;
		bool m_showVoxel = false;
		bool m_showTransparent = false;
		bool m_showVolumetric = false;
		bool m_saveScreenCapture = false;
		bool m_drawBRDFTest = false;

		GPUResourceComponent* m_Canvas;
		RenderPassComponent* m_CanvasOwner;
		VXGIRendererSystemConfig m_VXGIRendererSystemConfig = {};

		bool m_ExecuteOneShotCommands = true;

		void AuditDump();

	private:
		ObjectStatus m_ObjectStatus;
	};

	bool DefaultRenderingClientImpl::Setup(IServiceConfig* systemConfig)
	{
		f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_H, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

		f_showProbe = [&]() { m_showProbe = !m_showProbe; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

		f_showVoxel = [&]() { m_showVoxel = !m_showVoxel; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_V, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVoxel });

		f_showTransparent = [&]() { m_showTransparent = !m_showTransparent; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showTransparent });

		f_showVolumetric = [&]() { m_showVolumetric = !m_showVolumetric; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_J, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVolumetric });

		f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_C, true }, ButtonEvent{ EventLifeTime::OneShot, &f_saveScreenCapture });

		BRDFLUTPass::Get().Setup();
		BRDFLUTMSPass::Get().Setup();

		SunShadowCullingPass::Get().Setup();
		SunShadowGeometryProcessPass::Get().Setup();

		OpaqueCullingPass::Get().Setup();
		OpaquePass::Get().Setup();

		RadianceCacheReprojectionPass::Get().Setup();
		RadianceCacheRaytracingPass::Get().Setup();
		RadianceCacheFilterHorizontalPass::Get().Setup();
		RadianceCacheFilterVerticalPass::Get().Setup();
		RadianceCacheIntegrationPass::Get().Setup();

		SSAOPass::Get().Setup();

		TiledFrustumGenerationPass::Get().Setup();
		LightCullingPass::Get().Setup();

		LightPass::Get().Setup();

		SkyPass::Get().Setup();

		PreTAAPass::Get().Setup();
		TAAPass::Get().Setup();
		PostTAAPass::Get().Setup();

		LuminanceHistogramPass::Get().Setup();
		LuminanceAveragePass::Get().Setup();

		FinalBlendPass::Get().Setup();

		// SunShadowBlurOddPass::Get().Setup();
		// SunShadowBlurEvenPass::Get().Setup();

		// VXGIRenderer::Get().Setup(&m_VXGIRendererSystemConfig);

		// AnimationPass::Get().Setup();

		// TransparentGeometryProcessPass::Get().Setup();
		// TransparentBlendPass::Get().Setup();
		// VolumetricPass::Setup();

		// MotionBlurPass::Get().Setup();
		// BillboardPass::Get().Setup();
		// DebugPass::Get().Setup();

		// BSDFTestPass::Get().Setup();

		auto f_getUserPipelineOutputFunc = [this]()
			{
				return m_Canvas;
			};

		auto l_graphicsService = g_Engine->getGraphicsService();

		l_graphicsService->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}

	bool DefaultRenderingClientImpl::Initialize()
	{
		auto l_graphicsService = g_Engine->getGraphicsService();

		// GIDataLoader::Initialize();
		BRDFLUTPass::Get().Initialize();
		BRDFLUTMSPass::Get().Initialize();

		SunShadowCullingPass::Get().Initialize();
		SunShadowGeometryProcessPass::Get().Initialize();

		OpaqueCullingPass::Get().Initialize();
		OpaquePass::Get().Initialize();

		RadianceCacheReprojectionPass::Get().Initialize();
		RadianceCacheRaytracingPass::Get().Initialize();
		RadianceCacheFilterHorizontalPass::Get().Initialize();
		RadianceCacheFilterVerticalPass::Get().Initialize();
		RadianceCacheIntegrationPass::Get().Initialize();

		SSAOPass::Get().Initialize();

		TiledFrustumGenerationPass::Get().Initialize();
		LightCullingPass::Get().Initialize();

		LightPass::Get().Initialize();

		SkyPass::Get().Initialize();

		PreTAAPass::Get().Initialize();
		TAAPass::Get().Initialize();
		PostTAAPass::Get().Initialize();

		LuminanceHistogramPass::Get().Initialize();
		LuminanceAveragePass::Get().Initialize();

		FinalBlendPass::Get().Initialize();

		m_ObjectStatus = ObjectStatus::Activated;

		return true;
	}

	bool DefaultRenderingClientImpl::Update()
	{
		RadianceCacheReprojectionPass::Get().Update();
		TiledFrustumGenerationPass::Get().Update();
		LightCullingPass::Get().Update();
		LuminanceAveragePass::Get().Update();

		return true;
	}

	bool DefaultRenderingClientImpl::PrepareCommands()
	{
		m_Canvas = FinalBlendPass::Get().GetResult();
		m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();

		auto l_graphicsService = g_Engine->getGraphicsService();

		if (m_ExecuteOneShotCommands)
		{
			BRDFLUTPass::Get().PrepareCommandList();
			BRDFLUTMSPass::Get().PrepareCommandList();
		}

		SunShadowCullingPass::Get().PrepareCommandList();
		SunShadowGeometryProcessPass::Get().PrepareCommandList();

		OpaqueCullingPass::Get().PrepareCommandList();
		OpaquePass::Get().PrepareCommandList();

		RadianceCacheReprojectionPass::Get().PrepareCommandList();
		RadianceCacheRaytracingPass::Get().PrepareCommandList();
		RadianceCacheFilterHorizontalPass::Get().PrepareCommandList();
		RadianceCacheFilterVerticalPass::Get().PrepareCommandList();
		RadianceCacheIntegrationPass::Get().PrepareCommandList();

		SSAOPass::Get().PrepareCommandList();

		TiledFrustumGenerationPass::Get().PrepareCommandList();

		LightCullingPass::Get().PrepareCommandList();

		LightPass::Get().PrepareCommandList();

		SkyPass::Get().PrepareCommandList();

		PreTAAPass::Get().PrepareCommandList();

		TAAPassRenderingContext l_TAAPassRenderingContext;

		if (m_showLightHeatmap)
		{
			l_TAAPassRenderingContext.m_input = LightCullingPass::Get().GetHeatMap();
		}
		else if (m_showProbe)
		{
			l_TAAPassRenderingContext.m_input = RadianceCacheReprojectionPass::Get().GetCurrentFrameResult();
		}
		else
		{
			l_TAAPassRenderingContext.m_input = PreTAAPass::Get().GetResult();
		}

		l_TAAPassRenderingContext.m_motionVector = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3];

		TAAPass::Get().PrepareCommandList(&l_TAAPassRenderingContext);

		PostTAAPassRenderingContext l_PostTAAPassRenderingContext;
		l_PostTAAPassRenderingContext.m_input = TAAPass::Get().GetResult();
		PostTAAPass::Get().PrepareCommandList(&l_PostTAAPassRenderingContext);

		LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
		l_LuminanceHistogramPassRenderingContext.m_input = PostTAAPass::Get().GetResult();
		LuminanceHistogramPass::Get().PrepareCommandList(&l_LuminanceHistogramPassRenderingContext);

		LuminanceAveragePass::Get().PrepareCommandList();

		FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
		l_FinalBlendPassRenderingContext.m_input = PostTAAPass::Get().GetResult();
		FinalBlendPass::Get().PrepareCommandList(&l_FinalBlendPassRenderingContext);

		return true;
	}

	bool DefaultRenderingClientImpl::ExecuteCommands(IRenderingConfig* renderingConfig)
	{
		auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
		auto l_graphicsService = g_Engine->getGraphicsService();
		GPUResourceComponent* l_canvas;
		RenderPassComponent* l_canvasOwner;
		if (m_ExecuteOneShotCommands)
		{
			if (BRDFLUTPass::Get().GetStatus() == ObjectStatus::Activated
				&& BRDFLUTMSPass::Get().GetStatus() == ObjectStatus::Activated)
			{
				auto l_brdfRenderPass = BRDFLUTPass::Get().GetRenderPassComp();

				auto l_brdfPassComputeCommandList = BRDFLUTPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_graphicsService->Execute(l_brdfPassComputeCommandList, GPUEngineType::Compute);

				l_graphicsService->SignalOnGPU(l_brdfRenderPass, GPUEngineType::Compute);
				l_graphicsService->WaitOnGPU(l_brdfRenderPass, GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_brdfMSCommandList = BRDFLUTMSPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_graphicsService->Execute(l_brdfMSCommandList, GPUEngineType::Compute);
				auto l_brdfMSRenderPass = BRDFLUTMSPass::Get().GetRenderPassComp();
				l_graphicsService->SignalOnGPU(l_brdfMSRenderPass, GPUEngineType::Compute);

				auto l_graphicsSemaphoreValue = l_graphicsService->GetSemaphoreValue(GPUEngineType::Graphics);
				auto l_computeSemaphoreValue = l_graphicsService->GetSemaphoreValue(GPUEngineType::Compute);
				l_graphicsService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
				l_graphicsService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
				m_ExecuteOneShotCommands = false;
			}
		}

		if (SunShadowCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = SunShadowCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = SunShadowCullingPass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SunShadowGeometryProcessPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(SunShadowCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			auto l_commandList = SunShadowGeometryProcessPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Graphics);
			auto l_renderPass = SunShadowGeometryProcessPass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
		}

		if (OpaqueCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = OpaqueCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = OpaqueCullingPass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (OpaquePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(OpaqueCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			auto l_commandList = OpaquePass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Graphics);
			auto l_renderPass = OpaquePass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
		}

		if (RadianceCacheReprojectionPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = RadianceCacheReprojectionPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheRaytracingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(RadianceCacheReprojectionPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheRaytracingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			auto l_computeCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterHorizontalPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(RadianceCacheRaytracingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterHorizontalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(RadianceCacheFilterHorizontalPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterVerticalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(RadianceCacheFilterVerticalPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheIntegrationPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSAOPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = SSAOPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TiledFrustumGenerationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = TiledFrustumGenerationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = TiledFrustumGenerationPass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = LightCullingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_graphicsService->WaitOnGPU(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated)
				l_graphicsService->WaitOnGPU(RadianceCacheIntegrationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			
			auto l_renderPass = LightPass::Get().GetRenderPassComp();
			
			auto l_graphicsCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			
			auto l_computeCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SkyPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = SkyPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = SkyPass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (PreTAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(LightPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_graphicsService->WaitOnGPU(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = PreTAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = TAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (PostTAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = PostTAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = PostTAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = PostTAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LuminanceHistogramPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = LuminanceHistogramPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LuminanceAveragePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			auto l_commandList = LuminanceAveragePass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = LuminanceAveragePass::Get().GetRenderPassComp();
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (FinalBlendPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_graphicsService->WaitOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_graphicsService->WaitOnGPU(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = FinalBlendPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_graphicsService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_graphicsService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (m_saveScreenCapture)
		{
			auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_textureData = l_graphicsService->ReadTextureBackToCPU(FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
			g_Engine->Get<AssetService>()->Save("ScreenCapture", l_srcTextureComp->m_TextureDesc, l_textureData.data());
			m_saveScreenCapture = false;
		}

		if (g_Engine->getInitConfig().isAudit)
		{
			static uint32_t s_AuditFrame = 0;
			if (++s_AuditFrame == 5)
				AuditDump();
		}

		return true;
	}

	void DefaultRenderingClientImpl::AuditDump()
	{
		auto l_rs = g_Engine->getGraphicsService();

		l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
		l_rs->WaitOnCPU(l_rs->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);

		auto Dump = [&](const char* filename, RenderPassComponent* rp, TextureComponent* tc)
		{
			if (!tc) { Log(Warning, "AuditDump: null TextureComponent for ", filename); return; }
			auto l_pixels = l_rs->ReadTextureBackToCPU(rp, tc);
			if (l_pixels.empty()) { Log(Error, "AuditDump: empty readback for ", filename); return; }
			TextureDesc l_desc = tc->m_TextureDesc;
			l_desc.PixelDataType = TexturePixelDataType::Float32;
			g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
			Log(Success, "AuditDump: saved ", filename);
		};

		auto DumpRP = [&](const char* filename, RenderPassComponent* rp, uint32_t colorIndex = 0)
		{
			if (!rp || !rp->m_OutputMergerTarget) { Log(Warning, "AuditDump: null RenderPassComp for ", filename); return; }
			if (colorIndex >= rp->m_OutputMergerTarget->m_ColorOutputs.size()) { Log(Warning, "AuditDump: RT index ", colorIndex, " out of range for ", filename); return; }
			Dump(filename, rp, rp->m_OutputMergerTarget->m_ColorOutputs[colorIndex]);
		};

		// 1-2: BRDF LUTs
		Dump("audit_01_BRDFLUTPass.hdr",   BRDFLUTPass::Get().GetRenderPassComp(),   static_cast<TextureComponent*>(BRDFLUTPass::Get().GetResult()));
		Dump("audit_02_BRDFLUTMSPass.hdr",  BRDFLUTMSPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(BRDFLUTMSPass::Get().GetResult()));

		// 3: Shadow map
		DumpRP("audit_03_SunShadow_RT0.hdr", SunShadowGeometryProcessPass::Get().GetRenderPassComp(), 0);

		// 4: Opaque G-buffer
		DumpRP("audit_04a_Opaque_RT0.hdr", OpaquePass::Get().GetRenderPassComp(), 0);
		DumpRP("audit_04b_Opaque_RT1.hdr", OpaquePass::Get().GetRenderPassComp(), 1);
		DumpRP("audit_04c_Opaque_RT2.hdr", OpaquePass::Get().GetRenderPassComp(), 2);

		// 5: SSAO
		Dump("audit_05_SSAO.hdr", SSAOPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(SSAOPass::Get().GetResult()));

		// 8: Light
		Dump("audit_08a_Light_Luminance.hdr",   LightPass::Get().GetRenderPassComp(), LightPass::Get().GetLuminanceResult());
		Dump("audit_08b_Light_Illuminance.hdr",  LightPass::Get().GetRenderPassComp(), LightPass::Get().GetIlluminanceResult());

		// 9: Sky
		Dump("audit_09_Sky.hdr",     SkyPass::Get().GetRenderPassComp(),  static_cast<TextureComponent*>(SkyPass::Get().GetResult()));

		// 10: TAA
		Dump("audit_10_TAAPass.hdr",  TAAPass::Get().GetRenderPassComp(),  static_cast<TextureComponent*>(TAAPass::Get().GetResult()));

		// 11: Final blend
		// PrepareSwapChainCommands (which runs before ExecuteCommands) speculatively
		// transitions FinalBlend result to SRV in the state tracker. At AuditDump time
		// the actual GPU state is UAV (compute wrote, swap chain hasn't executed yet).
		// Temporarily correct the tracker, dump, then restore so the swap chain CL works.
		{
			auto* l_fbTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_fbIdx = l_fbTex->m_TextureDesc.IsMultiBuffer ? l_rs->GetCurrentFrame() : 0u;
			auto l_savedState = l_fbTex->GetCurrentState(l_fbIdx);
			l_fbTex->SetCurrentState(l_fbIdx, l_fbTex->m_WriteState);
			Dump("audit_11_FinalBlend.hdr", FinalBlendPass::Get().GetRenderPassComp(), l_fbTex);
			l_fbTex->SetCurrentState(l_fbIdx, l_savedState);
		}

		Log(Success, "AuditDump complete. Check Bin/*.hdr");
	}

	bool DefaultRenderingClientImpl::Terminate()
	{
		auto l_graphicsService = g_Engine->getGraphicsService();
		auto l_graphicsSemaphoreValue = l_graphicsService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = l_graphicsService->GetSemaphoreValue(GPUEngineType::Compute);
		l_graphicsService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		l_graphicsService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);

		FinalBlendPass::Get().Terminate();

		LuminanceAveragePass::Get().Terminate();
		LuminanceHistogramPass::Get().Terminate();

		PostTAAPass::Get().Terminate();
		TAAPass::Get().Terminate();
		PreTAAPass::Get().Terminate();

		SkyPass::Get().Terminate();

		LightPass::Get().Terminate();

		LightCullingPass::Get().Terminate();
		TiledFrustumGenerationPass::Get().Terminate();

		SSAOPass::Get().Terminate();

		RadianceCacheIntegrationPass::Get().Terminate();
		RadianceCacheFilterHorizontalPass::Get().Terminate();
		RadianceCacheFilterVerticalPass::Get().Terminate();
		RadianceCacheRaytracingPass::Get().Terminate();
		RadianceCacheReprojectionPass::Get().Terminate();

		OpaqueCullingPass::Get().Terminate();
		OpaquePass::Get().Terminate();

		SunShadowGeometryProcessPass::Get().Terminate();

		BRDFLUTMSPass::Get().Terminate();
		BRDFLUTPass::Get().Terminate();

		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus DefaultRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}

bool DefaultRenderingClient::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new DefaultRenderingClientImpl();
	return m_Impl->Setup(systemConfig);
}

bool DefaultRenderingClient::Initialize()
{
	return m_Impl->Initialize();
}

bool DefaultRenderingClient::Update()
{
	return m_Impl->Update();
}

bool DefaultRenderingClient::PrepareCommands()
{
	return m_Impl->PrepareCommands();
}

bool DefaultRenderingClient::ExecuteCommands(IRenderingConfig* renderingConfig)
{
	return m_Impl->ExecuteCommands(renderingConfig);
}

bool DefaultRenderingClient::Terminate()
{
	if (m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

ObjectStatus DefaultRenderingClient::GetStatus()
{
	return m_Impl->GetStatus();
}