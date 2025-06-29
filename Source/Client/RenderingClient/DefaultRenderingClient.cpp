#pragma once
#include "DefaultRenderingClient.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"
#include "SunShadowBlurEvenPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheFilterPass.h"
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
#include "../../Engine/Services/RenderingContextService.h"
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
		bool Setup(ISystemConfig* systemConfig) override;
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

	private:
		ObjectStatus m_ObjectStatus;
	};

	bool DefaultRenderingClientImpl::Setup(ISystemConfig* systemConfig)
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

		SunShadowGeometryProcessPass::Get().Setup();

		OpaquePass::Get().Setup();

		RadianceCacheReprojectionPass::Get().Setup();
		RadianceCacheRaytracingPass::Get().Setup();
		RadianceCacheFilterPass::Get().Setup();
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

		auto l_renderingServer = g_Engine->getRenderingServer();

		l_renderingServer->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}

	bool DefaultRenderingClientImpl::Initialize()
	{
		auto l_renderingServer = g_Engine->getRenderingServer();

		// GIDataLoader::Initialize();
		BRDFLUTPass::Get().Initialize();
		BRDFLUTMSPass::Get().Initialize();

		SunShadowGeometryProcessPass::Get().Initialize();

		OpaquePass::Get().Initialize();

		RadianceCacheReprojectionPass::Get().Initialize();
		RadianceCacheRaytracingPass::Get().Initialize();
		RadianceCacheFilterPass::Get().Initialize();
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

		// SunShadowBlurOddPass::Get().Initialize();
		// SunShadowBlurEvenPass::Get().Initialize();
		// VXGIRenderer::Get().Initialize();
		// AnimationPass::Get().Initialize();

		// TransparentGeometryProcessPass::Get().Initialize();
		// TransparentBlendPass::Get().Initialize();
		// VolumetricPass::Initialize();

		// MotionBlurPass::Get().Initialize();
		// BillboardPass::Get().Initialize();
		// DebugPass::Get().Initialize();

		// BSDFTestPass::Get().Initialize();
		m_ObjectStatus = ObjectStatus::Activated;

		return true;
	}

	bool DefaultRenderingClientImpl::Update()
	{
		SunShadowGeometryProcessPass::Get().Update();
		OpaquePass::Get().Update();
		RadianceCacheReprojectionPass::Get().Update();
		TiledFrustumGenerationPass::Get().Update();
		LightCullingPass::Get().Update();

		return true;
	}

	bool DefaultRenderingClientImpl::PrepareCommands()
	{
		m_Canvas = FinalBlendPass::Get().GetResult();
		m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();		

		auto l_renderingServer = g_Engine->getRenderingServer();

		// The blurring would cause precision errors and then flickering when camera rotates
		// SunShadowBlurOddPass::Get().PrepareCommandList();
		// SunShadowBlurEvenPass::Get().PrepareCommandList();

		if (m_ExecuteOneShotCommands)
		{
			BRDFLUTPass::Get().PrepareCommandList();
			BRDFLUTMSPass::Get().PrepareCommandList();
		}

		SunShadowGeometryProcessPass::Get().PrepareCommandList();

		OpaquePass::Get().PrepareCommandList();

		RadianceCacheReprojectionPass::Get().PrepareCommandList();
		RadianceCacheRaytracingPass::Get().PrepareCommandList();
		RadianceCacheFilterPass::Get().PrepareCommandList();
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
		auto l_renderingServer = g_Engine->getRenderingServer();
		GPUResourceComponent* l_canvas;
		RenderPassComponent* l_canvasOwner;
		if (m_ExecuteOneShotCommands)
		{
			if (BRDFLUTPass::Get().GetStatus() == ObjectStatus::Activated
				&& BRDFLUTMSPass::Get().GetStatus() == ObjectStatus::Activated)
			{
				l_renderingServer->ExecuteCommandList(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_renderingServer->SignalOnGPU(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_renderingServer->WaitOnGPU(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
				l_renderingServer->ExecuteCommandList(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_renderingServer->SignalOnGPU(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				auto l_graphicsSemaphoreValue = l_renderingServer->GetSemaphoreValue(GPUEngineType::Graphics);
				auto l_computeSemaphoreValue = l_renderingServer->GetSemaphoreValue(GPUEngineType::Compute);
				l_renderingServer->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
				l_renderingServer->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
				m_ExecuteOneShotCommands = false;
			}
		}

		if (SunShadowGeometryProcessPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->ExecuteCommandList(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->SignalOnGPU(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		}

		if (OpaquePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->ExecuteCommandList(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->SignalOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		}

		if (RadianceCacheReprojectionPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(RadianceCacheReprojectionPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(RadianceCacheReprojectionPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (RadianceCacheRaytracingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);			
			l_renderingServer->WaitOnGPU(RadianceCacheReprojectionPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(RadianceCacheRaytracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(RadianceCacheRaytracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (RadianceCacheFilterPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(RadianceCacheRaytracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			// Execute horizontal pass first
			l_renderingServer->ExecuteCommandList(RadianceCacheFilterPass::Get().GetHorizontalRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(RadianceCacheFilterPass::Get().GetHorizontalRenderPassComp(), GPUEngineType::Compute);
			// Wait for horizontal pass completion, then execute vertical pass
			l_renderingServer->WaitOnGPU(RadianceCacheFilterPass::Get().GetHorizontalRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(RadianceCacheFilterPass::Get().GetVerticalRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(RadianceCacheFilterPass::Get().GetVerticalRenderPassComp(), GPUEngineType::Compute);
		}

		if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(RadianceCacheFilterPass::Get().GetVerticalRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(RadianceCacheIntegrationPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(RadianceCacheIntegrationPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (SSAOPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (TiledFrustumGenerationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->ExecuteCommandList(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (LightCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (LightPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(RadianceCacheIntegrationPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->WaitOnGPU(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(LightPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(LightPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (SkyPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->ExecuteCommandList(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (PreTAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(LightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (TAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->WaitOnGPU(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (PostTAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (LuminanceHistogramPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (LuminanceAveragePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		if (FinalBlendPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_renderingServer->WaitOnGPU(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->WaitOnGPU(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->SignalOnGPU(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		}

		// l_renderingServer->WaitOnGPU(l_canvasOwner, GPUEngineType::Graphics,  l_canvasOwner->m_RenderPassDesc.m_GPUEngineType);
		// l_renderingServer->WaitOnGPU(BillboardPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
		// l_renderingServer->WaitOnGPU(DebugPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

		// l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// l_renderingServer->WaitOnGPU(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

		// l_renderingServer->WaitOnGPU(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
		// l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// l_renderingServer->WaitOnGPU(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

		// m_VXGIRendererSystemConfig.m_VXGIRenderingConfig.m_visualize = m_showVoxel;
		// VXGIRenderer::Get().ExecuteCommands(&m_VXGIRendererSystemConfig.m_VXGIRenderingConfig);

		// if (m_drawBRDFTest)
		// {
		// 	BSDFTestPass::Get().PrepareCommandList();
		// 	l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// 	l_renderingServer->WaitOnGPU(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// 	l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		// 	l_canvas = BSDFTestPass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture;
		// 	l_canvasOwner = BSDFTestPass::Get().GetRenderPassComp();
		// }
		// else if (m_showVolumetric)
		// {
		// 	VolumetricPass::ExecuteCommands(true);
		// 	l_canvas = VolumetricPass::GetVisualizationResult();
		// }
		// else
		// {
		// 	AnimationPass::Get().PrepareCommandList();

		// 	l_renderingServer->ExecuteCommandList(AnimationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);

		// 	l_renderingServer->WaitOnGPU(AnimationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

		// 	//VolumetricPass::ExecuteCommands(false);

		// 	if (m_showTransparent)
		// 	{
		// 		TransparentGeometryProcessPass::Get().PrepareCommandList();

		// 		TransparentBlendPassRenderingContext l_transparentBlendPassRenderingContext;
		// 		l_transparentBlendPassRenderingContext.m_output = l_canvas;
		// 		TransparentBlendPass::Get().PrepareCommandList(&l_transparentBlendPassRenderingContext);
		// 	}

		// 	//l_renderingServer->WaitOnGPU(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

		// 	if (m_showTransparent)
		// 	{
		// 		l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// 		l_renderingServer->WaitOnGPU(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// 		l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		// 		l_renderingServer->WaitOnGPU(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
		// 		l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// 		l_renderingServer->WaitOnGPU(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// 		l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		// 	}
		// }

		// if (m_showVoxel)
		// {
		// 	l_canvas = VXGIRenderer::Get().GetVisualizationResult();
		// }

		// if (m_showTransparent)
		// {
		// 	l_renderingServer->WaitOnGPU(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
		// }

		// }

		// if (l_renderingConfig.useMotionBlur)
		// {
		// 	MotionBlurPassRenderingContext l_MotionBlurPassRenderingContext;
		// 	l_MotionBlurPassRenderingContext.m_input = l_canvas;
		// 	MotionBlurPass::Get().PrepareCommandList(&l_MotionBlurPassRenderingContext);

		// 	l_renderingServer->WaitOnGPU(l_canvasOwner, GPUEngineType::Graphics,  l_canvasOwner->m_RenderPassDesc.m_GPUEngineType);
		// 	l_renderingServer->ExecuteCommandList(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// 	l_renderingServer->WaitOnGPU(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		// 	l_renderingServer->ExecuteCommandList(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

		// 	l_canvas = MotionBlurPass::Get().GetResult();
		// 	l_canvasOwner = MotionBlurPass::Get().GetRenderPassComp();
		// }

		// BillboardPass::Get().PrepareCommandList();
		// DebugPass::Get().PrepareCommandList();

		// l_renderingServer->ExecuteCommandList(BillboardPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		// l_renderingServer->ExecuteCommandList(DebugPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);

		if (m_saveScreenCapture)
		{
			auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_textureData = l_renderingServer->ReadTextureBackToCPU(FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
			g_Engine->Get<AssetService>()->SaveTexture("ScreenCapture", l_srcTextureComp->m_TextureDesc, l_textureData.data());
			m_saveScreenCapture = false;
		}

		return true;
	}

	bool DefaultRenderingClientImpl::Terminate()
	{
		auto l_renderingServer = g_Engine->getRenderingServer();
		auto l_graphicsSemaphoreValue = l_renderingServer->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = l_renderingServer->GetSemaphoreValue(GPUEngineType::Compute);
		l_renderingServer->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		l_renderingServer->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);

		// BSDFTestPass::Get().Terminate();

		FinalBlendPass::Get().Terminate();

		LuminanceAveragePass::Get().Terminate();
		LuminanceHistogramPass::Get().Terminate();

		// DebugPass::Get().Terminate();
		// BillboardPass::Get().Terminate();
		// MotionBlurPass::Get().Terminate();

		// VolumetricPass::Terminate();
		// TransparentBlendPass::Get().Terminate();
		// TransparentGeometryProcessPass::Get().Terminate();

		PostTAAPass::Get().Terminate();
		TAAPass::Get().Terminate();
		PreTAAPass::Get().Terminate();

		SkyPass::Get().Terminate();

		LightPass::Get().Terminate();

		LightCullingPass::Get().Terminate();
		TiledFrustumGenerationPass::Get().Terminate();

		// AnimationPass::Get().Terminate();
		SSAOPass::Get().Terminate();

		RadianceCacheIntegrationPass::Get().Terminate();
		RadianceCacheFilterPass::Get().Terminate();
		RadianceCacheRaytracingPass::Get().Terminate();
		RadianceCacheReprojectionPass::Get().Terminate();
		
		OpaquePass::Get().Terminate();
		// VXGIRenderer::Get().Terminate();

		SunShadowGeometryProcessPass::Get().Terminate();

		BRDFLUTMSPass::Get().Terminate();
		BRDFLUTPass::Get().Terminate();

		// SunShadowBlurEvenPass::Get().Terminate();
		// SunShadowBlurOddPass::Get().Terminate();

		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus DefaultRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}

bool DefaultRenderingClient::Setup(ISystemConfig* systemConfig)
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