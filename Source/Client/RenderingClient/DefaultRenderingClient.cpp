#include "DefaultRenderingClient.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "LightCullingPass.h"
#include "GIDataLoader.h"
#include "GIResolvePass.h"
#include "GIResolveTestPass.h"
#include "LuminanceHistogramPass.h"
#include "BRDFLUTPass.h"
#include "SunShadowPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentPass.h"
#include "VolumetricPass.h"
#include "VoxelizationPass.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

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
}

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup()
{
	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_H, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

	f_showProbe = [&]() { m_showProbe = !m_showProbe; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

	f_showVoxel = [&]() { m_showVoxel = !m_showVoxel; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_V, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVoxel });

	f_showTransparent = [&]() { m_showTransparent = !m_showTransparent; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showTransparent });

	f_showVolumetric = [&]() { m_showVolumetric = !m_showVolumetric; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_J, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVolumetric });

	f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_C, true }, ButtonEvent{ EventLifeTime::OneShot, &f_saveScreenCapture });

	f_SetupJob = [&]()
	{
		DefaultGPUBuffers::Setup();
		GIDataLoader::Setup();
		BRDFLUTPass::Setup();

		LightCullingPass::Setup();
		GIResolvePass::Setup();
		GIResolveTestPass::Setup();
		LuminanceHistogramPass::Setup();

		SunShadowPass::Setup();
		VoxelizationPass::Setup();
		OpaquePass::Setup();
		AnimationPass::Setup();

		SSAOPass::Setup();
		LightPass::Setup();
		SkyPass::Setup();
		PreTAAPass::Setup();
		TransparentPass::Setup();
		VolumetricPass::Setup();
		TAAPass::Setup();
		PostTAAPass::Setup();
		MotionBlurPass::Setup();
		BillboardPass::Setup();
		DebugPass::Setup();
		FinalBlendPass::Setup();

		BSDFTestPass::Setup();
	};

	f_InitializeJob = [&]()
	{
		DefaultGPUBuffers::Initialize();
		GIDataLoader::Initialize();
		BRDFLUTPass::Initialize();
		BRDFLUTPass::PrepareCommandList();
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(BRDFLUTPass::GetBRDFLUTRPDC());
		g_pModuleManager->getRenderingServer()->WaitForFrame(BRDFLUTPass::GetBRDFLUTRPDC());
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(BRDFLUTPass::GetBRDFMSLUTRPDC());
		g_pModuleManager->getRenderingServer()->WaitForFrame(BRDFLUTPass::GetBRDFMSLUTRPDC());

		LightCullingPass::Initialize();
		LuminanceHistogramPass::Initialize();
		GIResolvePass::Initialize();
		GIResolveTestPass::Initialize();

		SunShadowPass::Initialize();
		VoxelizationPass::Initialize();
		OpaquePass::Initialize();
		AnimationPass::Initialize();

		SSAOPass::Initialize();
		LightPass::Initialize();
		SkyPass::Initialize();
		PreTAAPass::Initialize();
		TransparentPass::Initialize();
		VolumetricPass::Initialize();
		TAAPass::Initialize();
		PostTAAPass::Initialize();
		MotionBlurPass::Initialize();
		BillboardPass::Initialize();
		DebugPass::Initialize();
		FinalBlendPass::Initialize();

		BSDFTestPass::Initialize();
	};

	f_RenderJob = [&]()
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();
		IResourceBinder* l_canvas;

		DefaultGPUBuffers::Upload();

		LightCullingPass::PrepareCommandList();
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(LightCullingPass::GetTileFrustumRPDC());
		g_pModuleManager->getRenderingServer()->WaitForFrame(LightCullingPass::GetTileFrustumRPDC());
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(LightCullingPass::GetLightCullingRPDC());
		g_pModuleManager->getRenderingServer()->WaitForFrame(LightCullingPass::GetLightCullingRPDC());

		SunShadowPass::PrepareCommandList();
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetSunShadowRPDC());
		g_pModuleManager->getRenderingServer()->WaitForFrame(SunShadowPass::GetSunShadowRPDC());
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetBlurRPDCOdd());
		g_pModuleManager->getRenderingServer()->WaitForFrame(SunShadowPass::GetBlurRPDCOdd());
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetBlurRPDCEven());
		g_pModuleManager->getRenderingServer()->WaitForFrame(SunShadowPass::GetBlurRPDCEven());

		VoxelizationPass::Render(m_showVoxel);

		if (m_showVoxel)
		{
			l_canvas = VoxelizationPass::GetVisualizationResult();
		}
		else if (m_drawBRDFTest)
		{
			BSDFTestPass::Render();
			l_canvas = BSDFTestPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}
		else if (m_showLightHeatmap)
		{
			l_canvas = LightCullingPass::GetHeatMap();
		}
		else if (m_showProbe)
		{
			GIResolveTestPass::Render();
			l_canvas = GIResolveTestPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}
		else if (m_showTransparent)
		{
			TransparentPass::Render(0);
			l_canvas = TransparentPass::GetResult();
		}
		else if (m_showVolumetric)
		{
			VolumetricPass::Render(true);
			l_canvas = VolumetricPass::GetVisualizationResult();
		}
		else
		{
			//GIResolvePass::PrepareCommandList();

			OpaquePass::PrepareCommandList();
			AnimationPass::PrepareCommandList();

			g_pModuleManager->getRenderingServer()->ExecuteCommandList(OpaquePass::GetRPDC());
			g_pModuleManager->getRenderingServer()->WaitForFrame(OpaquePass::GetRPDC());
			g_pModuleManager->getRenderingServer()->ExecuteCommandList(AnimationPass::GetRPDC());
			g_pModuleManager->getRenderingServer()->WaitForFrame(AnimationPass::GetRPDC());

			SSAOPass::Render();

			VolumetricPass::Render(false);

			LightPass::PrepareCommandList();
			if (l_renderingConfig.drawSky)
			{
				SkyPass::PrepareCommandList();
			}
			PreTAAPass::PrepareCommandList();

			g_pModuleManager->getRenderingServer()->ExecuteCommandList(LightPass::GetRPDC());
			g_pModuleManager->getRenderingServer()->ExecuteCommandList(SkyPass::GetRPDC());

			g_pModuleManager->getRenderingServer()->WaitForFrame(LightPass::GetRPDC());
			g_pModuleManager->getRenderingServer()->WaitForFrame(SkyPass::GetRPDC());

			g_pModuleManager->getRenderingServer()->ExecuteCommandList(PreTAAPass::GetRPDC());
			g_pModuleManager->getRenderingServer()->WaitForFrame(PreTAAPass::GetRPDC());

			TransparentPass::Render(PreTAAPass::GetRPDC()->m_RenderTargetsResourceBinders[0]);
			l_canvas = PreTAAPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		LuminanceHistogramPass::Render(l_canvas);

		if (l_renderingConfig.useTAA)
		{
			TAAPass::Render(l_canvas);
			PostTAAPass::Render();
			l_canvas = PostTAAPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::Render(l_canvas);
			l_canvas = MotionBlurPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		BillboardPass::Render();
		DebugPass::Render();

		FinalBlendPass::Render(l_canvas);

		if (m_saveScreenCapture)
		{
			auto l_textureData = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(FinalBlendPass::GetRPDC(), FinalBlendPass::GetRPDC()->m_RenderTargets[0]);
			auto l_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
			l_TDC->m_TextureDesc = FinalBlendPass::GetRPDC()->m_RenderTargets[0]->m_TextureDesc;
			l_TDC->m_TextureData = l_textureData.data();
			g_pModuleManager->getAssetSystem()->saveTexture("ScreenCapture", l_TDC);
			//g_pModuleManager->getRenderingServer()->DeleteTextureDataComponent(l_TDC);
			m_saveScreenCapture = false;
		}
	};

	f_TerminateJob = [&]()
	{
		DefaultGPUBuffers::Terminate();
		LightCullingPass::Terminate();
		GIResolvePass::Terminate();
		GIResolveTestPass::Terminate();
		LuminanceHistogramPass::Terminate();
		GIDataLoader::Terminate();
		BRDFLUTPass::Terminate();
		SunShadowPass::Terminate();
		VoxelizationPass::Terminate();
		OpaquePass::Terminate();
		AnimationPass::Terminate();
		SSAOPass::Terminate();
		LightPass::Terminate();
		SkyPass::Terminate();
		PreTAAPass::Terminate();
		TransparentPass::Terminate();
		VolumetricPass::Terminate();
		TAAPass::Terminate();
		PostTAAPass::Terminate();
		MotionBlurPass::Terminate();
		BillboardPass::Terminate();
		DebugPass::Terminate();
		FinalBlendPass::Terminate();

		BSDFTestPass::Terminate();
	};

	auto l_DefaultRenderingClientSetupTask = g_pModuleManager->getTaskSystem()->submit("DefaultRenderingClientSetupTask", 2, nullptr, f_SetupJob);
	l_DefaultRenderingClientSetupTask->Wait();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	auto l_DefaultRenderingClientInitializeTask = g_pModuleManager->getTaskSystem()->submit("DefaultRenderingClientInitializeTask", 2, nullptr, f_InitializeJob);
	l_DefaultRenderingClientInitializeTask->Wait();

	return true;
}

bool DefaultRenderingClient::Render()
{
	f_RenderJob();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	f_TerminateJob();

	return true;
}