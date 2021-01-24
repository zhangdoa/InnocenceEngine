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

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

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

bool DefaultRenderingClient::Setup(ISystemConfig* systemConfig)
{
	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_H, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

	f_showProbe = [&]() { m_showProbe = !m_showProbe; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

	f_showVoxel = [&]() { m_showVoxel = !m_showVoxel; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_V, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVoxel });

	f_showTransparent = [&]() { m_showTransparent = !m_showTransparent; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showTransparent });

	f_showVolumetric = [&]() { m_showVolumetric = !m_showVolumetric; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_J, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVolumetric });

	f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_C, true }, ButtonEvent{ EventLifeTime::OneShot, &f_saveScreenCapture });

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
		BRDFLUTPass::Render();

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
		auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();
		GPUResourceComponent* l_canvas;

		DefaultGPUBuffers::Upload();

		LightCullingPass::PrepareCommandList();
		g_Engine->getRenderingServer()->ExecuteCommandList(LightCullingPass::GetTileFrustumRPDC());
		g_Engine->getRenderingServer()->WaitForFrame(LightCullingPass::GetTileFrustumRPDC());
		g_Engine->getRenderingServer()->ExecuteCommandList(LightCullingPass::GetLightCullingRPDC());
		g_Engine->getRenderingServer()->WaitForFrame(LightCullingPass::GetLightCullingRPDC());

		SunShadowPass::PrepareCommandList();
		g_Engine->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetGeometryProcessRPDC());
		g_Engine->getRenderingServer()->WaitForFrame(SunShadowPass::GetGeometryProcessRPDC());
		g_Engine->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetBlurRPDCOdd());
		g_Engine->getRenderingServer()->WaitForFrame(SunShadowPass::GetBlurRPDCOdd());
		g_Engine->getRenderingServer()->ExecuteCommandList(SunShadowPass::GetBlurRPDCEven());
		g_Engine->getRenderingServer()->WaitForFrame(SunShadowPass::GetBlurRPDCEven());

		VoxelizationPass::Render(m_showVoxel, 0, false);

		if (m_drawBRDFTest)
		{
			BSDFTestPass::Render();
			l_canvas = BSDFTestPass::GetRPDC()->m_RenderTargets[0];
		}
		else if (m_showLightHeatmap)
		{
			l_canvas = LightCullingPass::GetHeatMap();
		}
		else if (m_showProbe)
		{
			GIResolveTestPass::Render();
			l_canvas = GIResolveTestPass::GetRPDC()->m_RenderTargets[0];
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

			g_Engine->getRenderingServer()->ExecuteCommandList(OpaquePass::GetRPDC());
			g_Engine->getRenderingServer()->WaitForFrame(OpaquePass::GetRPDC());
			g_Engine->getRenderingServer()->ExecuteCommandList(AnimationPass::GetRPDC());
			g_Engine->getRenderingServer()->WaitForFrame(AnimationPass::GetRPDC());

			SSAOPass::Render();

			VolumetricPass::Render(false);

			LightPass::PrepareCommandList();
			if (l_renderingConfig.drawSky)
			{
				SkyPass::PrepareCommandList();
			}
			PreTAAPass::PrepareCommandList();

			g_Engine->getRenderingServer()->ExecuteCommandList(LightPass::GetRPDC());
			g_Engine->getRenderingServer()->ExecuteCommandList(SkyPass::GetRPDC());

			g_Engine->getRenderingServer()->WaitForFrame(LightPass::GetRPDC());
			g_Engine->getRenderingServer()->WaitForFrame(SkyPass::GetRPDC());

			g_Engine->getRenderingServer()->ExecuteCommandList(PreTAAPass::GetRPDC());
			g_Engine->getRenderingServer()->WaitForFrame(PreTAAPass::GetRPDC());

			TransparentPass::Render(PreTAAPass::GetResult());
			l_canvas = PreTAAPass::GetResult();
		}

		if (m_showVoxel)
		{
			l_canvas = VoxelizationPass::GetVisualizationResult();
		}

		LuminanceHistogramPass::Render(l_canvas);

		if (l_renderingConfig.useTAA)
		{
			TAAPass::Render(l_canvas);
			PostTAAPass::Render();
			l_canvas = PostTAAPass::GetResult();
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::Render(l_canvas);
			l_canvas = MotionBlurPass::GetResult();
		}

		BillboardPass::Render();
		DebugPass::Render();

		FinalBlendPass::Render(l_canvas);

		if (m_saveScreenCapture)
		{
			auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

			auto l_textureData = g_Engine->getRenderingServer()->ReadTextureBackToCPU(FinalBlendPass::GetRPDC(), FinalBlendPass::GetRPDC()->m_RenderTargets[0]);
			auto l_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent();
			l_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
			l_TDC->m_TextureData = l_textureData.data();
			g_Engine->getAssetSystem()->saveTexture("ScreenCapture", l_TDC);
			//g_Engine->getRenderingServer()->DeleteTextureDataComponent(l_TDC);
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

	auto l_DefaultRenderingClientSetupTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientSetupTask", 2, nullptr, f_SetupJob);
	l_DefaultRenderingClientSetupTask.m_Future->Get();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	auto l_DefaultRenderingClientInitializeTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientInitializeTask", 2, nullptr, f_InitializeJob);
	l_DefaultRenderingClientInitializeTask.m_Future->Get();

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

ObjectStatus DefaultRenderingClient::GetStatus()
{
	return ObjectStatus();
}