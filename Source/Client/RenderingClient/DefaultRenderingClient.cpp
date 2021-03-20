#include "DefaultRenderingClient.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "GIDataLoader.h"
#include "GIResolvePass.h"
#include "SurfelGITestPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"
#include "SunShadowBlurEvenPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentGeometryProcessPass.h"
#include "TransparentBlendPass.h"
#include "VolumetricPass.h"
#include "VXGIRenderer.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

namespace DefaultRenderingClientNS
{
	std::function<void()> f_showLightHeatmap;
	std::function<void()> f_showProbe;
	std::function<void()> f_showVoxel;
	std::function<void()> f_showTransparent;
	std::function<void()> f_showVolumetric;
	std::function<void()> f_saveScreenCapture;

	std::function<void()> f_SetupJob;
	std::function<void()> f_InitializeJob;
	std::function<void()> f_RenderJob;
	std::function<void()> f_TerminateJob;

	bool m_showLightHeatmap = false;
	bool m_showProbe = false;
	bool m_showVoxel = false;
	bool m_showTransparent = false;
	bool m_showVolumetric = false;
	bool m_saveScreenCapture = false;
	static bool m_drawBRDFTest = false;
} // namespace DefaultRenderingClientNS

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup(ISystemConfig *systemConfig)
{
	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_H, true}, ButtonEvent{EventLifeTime::OneShot, &f_showLightHeatmap});

	f_showProbe = [&]() { m_showProbe = !m_showProbe; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_G, true}, ButtonEvent{EventLifeTime::OneShot, &f_showProbe});

	f_showVoxel = [&]() { m_showVoxel = !m_showVoxel; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_V, true}, ButtonEvent{EventLifeTime::OneShot, &f_showVoxel});

	f_showTransparent = [&]() { m_showTransparent = !m_showTransparent; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_T, true}, ButtonEvent{EventLifeTime::OneShot, &f_showTransparent});

	f_showVolumetric = [&]() { m_showVolumetric = !m_showVolumetric; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_J, true}, ButtonEvent{EventLifeTime::OneShot, &f_showVolumetric});

	f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{INNO_KEY_C, true}, ButtonEvent{EventLifeTime::OneShot, &f_saveScreenCapture});

	f_SetupJob = [&]() {
		DefaultGPUBuffers::Setup();
		GIDataLoader::Setup();
		BRDFLUTPass::Get().Setup();
		BRDFLUTMSPass::Get().Setup();

		LightCullingPass::Get().Setup();
		GIResolvePass::Setup();
		SurfelGITestPass::Get().Setup();
		LuminanceHistogramPass::Get().Setup();
		LuminanceAveragePass::Get().Setup();

		SunShadowGeometryProcessPass::Get().Setup();
		SunShadowBlurOddPass::Get().Setup();
		SunShadowBlurEvenPass::Get().Setup();
		VXGIRenderer::Get().Setup();
		OpaquePass::Get().Setup();
		AnimationPass::Get().Setup();

		SSAOPass::Get().Setup();
		LightPass::Get().Setup();
		SkyPass::Get().Setup();
		PreTAAPass::Get().Setup();
		TransparentGeometryProcessPass::Get().Setup();
		TransparentBlendPass::Get().Setup();
		VolumetricPass::Setup();
		TAAPass::Get().Setup();
		PostTAAPass::Get().Setup();
		MotionBlurPass::Get().Setup();
		BillboardPass::Get().Setup();
		DebugPass::Get().Setup();
		FinalBlendPass::Get().Setup();

		BSDFTestPass::Get().Setup();
	};

	f_InitializeJob = [&]() {
		auto l_renderingServer = g_Engine->getRenderingServer();

		DefaultGPUBuffers::Initialize();
		GIDataLoader::Initialize();
		BRDFLUTPass::Get().Initialize();
		BRDFLUTMSPass::Get().Initialize();

		BRDFLUTPass::Get().PrepareCommandList();
		BRDFLUTMSPass::Get().PrepareCommandList();
		l_renderingServer->ExecuteCommandList(BRDFLUTPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(BRDFLUTPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);
		l_renderingServer->ExecuteCommandList(BRDFLUTMSPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(BRDFLUTMSPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		LightCullingPass::Get().Initialize();
		GIResolvePass::Initialize();
		SurfelGITestPass::Get().Initialize();
		LuminanceHistogramPass::Get().Initialize();
		LuminanceAveragePass::Get().Initialize();

		SunShadowGeometryProcessPass::Get().Initialize();
		SunShadowBlurOddPass::Get().Initialize();
		SunShadowBlurEvenPass::Get().Initialize();
		VXGIRenderer::Get().Initialize();
		OpaquePass::Get().Initialize();
		AnimationPass::Get().Initialize();

		SSAOPass::Get().Initialize();
		LightPass::Get().Initialize();
		SkyPass::Get().Initialize();
		PreTAAPass::Get().Initialize();
		TransparentGeometryProcessPass::Get().Initialize();
		TransparentBlendPass::Get().Initialize();
		VolumetricPass::Initialize();
		TAAPass::Get().Initialize();
		PostTAAPass::Get().Initialize();
		MotionBlurPass::Get().Initialize();
		BillboardPass::Get().Initialize();
		DebugPass::Get().Initialize();
		FinalBlendPass::Get().Initialize();

		BSDFTestPass::Get().Initialize();
	};

	f_RenderJob = [&]() {
		auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();
		auto l_renderingServer = g_Engine->getRenderingServer();
		GPUResourceComponent *l_canvas;

		DefaultGPUBuffers::Upload();

		TiledFrustumGenerationPass::Get().PrepareCommandList();
		LightCullingPass::Get().PrepareCommandList();

		l_renderingServer->ExecuteCommandList(TiledFrustumGenerationPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(TiledFrustumGenerationPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		l_renderingServer->ExecuteCommandList(LightCullingPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(LightCullingPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		SunShadowGeometryProcessPass::Get().PrepareCommandList();
		SunShadowBlurOddPass::Get().PrepareCommandList();
		SunShadowBlurEvenPass::Get().PrepareCommandList();

		l_renderingServer->ExecuteCommandList(SunShadowGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(SunShadowGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		//VXGIRenderer::Render();

		if (m_drawBRDFTest)
		{
			BSDFTestPass::Get().PrepareCommandList();
			l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);
			l_canvas = BSDFTestPass::Get().GetRPDC()->m_RenderTargets[0];
		}
		else if (m_showLightHeatmap)
		{
			l_canvas = LightCullingPass::Get().GetHeatMap();
		}
		else if (m_showProbe)
		{
			SurfelGITestPass::Get().PrepareCommandList();
			l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);
			l_canvas = SurfelGITestPass::Get().GetRPDC()->m_RenderTargets[0];
		}
		else if (m_showTransparent)
		{
			TransparentGeometryProcessPass::Get().PrepareCommandList();
			TransparentBlendPass::Get().PrepareCommandList();
			l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);
			l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);			
			l_canvas = TransparentBlendPass::Get().GetResult();
		}
		else if (m_showVolumetric)
		{
			VolumetricPass::Render(true);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_canvas = VolumetricPass::GetVisualizationResult();
		}
		else
		{
			//GIResolvePass::PrepareCommandList();

			OpaquePass::Get().PrepareCommandList();
			AnimationPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(OpaquePass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);

			l_canvas = OpaquePass::Get().GetRPDC()->m_RenderTargets[0];
			l_renderingServer->ExecuteCommandList(AnimationPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);

			SSAOPass::Get().PrepareCommandList();

			VolumetricPass::Render(false);

			LightPass::Get().PrepareCommandList();

			if (l_renderingConfig.drawSky)
			{
				SkyPass::Get().PrepareCommandList();
			}
			PreTAAPass::Get().PrepareCommandList();
			TransparentGeometryProcessPass::Get().PrepareCommandList();
			TransparentBlendPass::Get().PrepareCommandList();
			
			l_renderingServer->ExecuteCommandList(LightPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			l_renderingServer->ExecuteCommandList(SkyPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			l_renderingServer->ExecuteCommandList(PreTAAPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);
			l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRPDC(), RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);	

			l_canvas = PreTAAPass::Get().GetResult();
		}

		if (m_showVoxel)
		{
			l_canvas = VXGIRenderer::Get().GetVisualizationResult();
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);
		}

		LuminanceHistogramPass::Get().PrepareCommandList();
		LuminanceAveragePass::Get().PrepareCommandList();

		l_renderingServer->ExecuteCommandList(LuminanceHistogramPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		l_renderingServer->ExecuteCommandList(LuminanceAveragePass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		if (l_renderingConfig.useTAA)
		{
			TAAPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(TAAPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			PostTAAPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(PostTAAPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			l_canvas = PostTAAPass::Get().GetResult();
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::Get().PrepareCommandList();
			l_renderingServer->ExecuteCommandList(MotionBlurPass::Get().GetRPDC(), RenderPassUsage::Compute);
			l_renderingServer->WaitFence(RenderPassUsage::Graphics);
			l_renderingServer->WaitFence(RenderPassUsage::Compute);

			l_canvas = MotionBlurPass::Get().GetResult();
		}

		BillboardPass::Get().PrepareCommandList();
		DebugPass::Get().PrepareCommandList();
		l_renderingServer->ExecuteCommandList(BillboardPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		l_renderingServer->ExecuteCommandList(DebugPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		FinalBlendPass::Get().PrepareCommandList();
		l_renderingServer->ExecuteCommandList(FinalBlendPass::Get().GetRPDC(), RenderPassUsage::Compute);
		l_renderingServer->WaitFence(RenderPassUsage::Graphics);
		l_renderingServer->WaitFence(RenderPassUsage::Compute);

		if (m_saveScreenCapture)
		{
			auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

			auto l_textureData = l_renderingServer->ReadTextureBackToCPU(FinalBlendPass::Get().GetRPDC(), FinalBlendPass::Get().GetRPDC()->m_RenderTargets[0]);
			auto l_TDC = l_renderingServer->AddTextureDataComponent();
			l_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
			l_TDC->m_TextureData = l_textureData.data();
			g_Engine->getAssetSystem()->saveTexture("ScreenCapture", l_TDC);
			//l_renderingServer->DeleteTextureDataComponent(l_TDC);
			m_saveScreenCapture = false;
		}
	};

	f_TerminateJob = [&]() {
		DefaultGPUBuffers::Terminate();

		BRDFLUTPass::Get().Terminate();
		BRDFLUTMSPass::Get().Terminate();

		LightCullingPass::Get().Terminate();
		GIResolvePass::Terminate();
		SurfelGITestPass::Get().Terminate();
		LuminanceHistogramPass::Get().Terminate();
		LuminanceAveragePass::Get().Terminate();

		SunShadowGeometryProcessPass::Get().Terminate();
		SunShadowBlurOddPass::Get().Terminate();
		SunShadowBlurEvenPass::Get().Terminate();
		VXGIRenderer::Get().Terminate();
		OpaquePass::Get().Terminate();
		AnimationPass::Get().Terminate();

		SSAOPass::Get().Terminate();
		LightPass::Get().Terminate();
		SkyPass::Get().Terminate();
		PreTAAPass::Get().Terminate();
		TransparentGeometryProcessPass::Get().Terminate();
		TransparentBlendPass::Get().Terminate();
		VolumetricPass::Terminate();
		TAAPass::Get().Terminate();
		PostTAAPass::Get().Terminate();
		MotionBlurPass::Get().Terminate();
		BillboardPass::Get().Terminate();
		DebugPass::Get().Terminate();
		FinalBlendPass::Get().Terminate();

		BSDFTestPass::Get().Terminate();
	};

	auto l_DefaultRenderingClientSetupTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientSetupTask", 2, nullptr, f_SetupJob);
	l_DefaultRenderingClientSetupTask.m_Future->Get();

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool DefaultRenderingClient::Initialize()
{
	auto l_DefaultRenderingClientInitializeTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientInitializeTask", 2, nullptr, f_InitializeJob);
	l_DefaultRenderingClientInitializeTask.m_Future->Get();
	
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DefaultRenderingClient::Render(IRenderingConfig* renderingConfig)
{
	//f_RenderJob();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	f_TerminateJob();
	
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus DefaultRenderingClient::GetStatus()
{
	return m_ObjectStatus;
}