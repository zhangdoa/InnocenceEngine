#include "DefaultRenderingClient.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "LightCullingPass.h"
#include "GIDataLoader.h"
#include "GIResolvePass.h"
#include "GIResolveTestPass.h"
#include "BRDFLUTPass.h"
#include "SunShadowPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentPass.h"
#include "VolumetricFogPass.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace DefaultRenderingClientNS
{
	std::function<void()> f_showLightHeatmap;
	std::function<void()> f_showProbe;
	std::function<void()> f_saveScreenCapture;

	std::function<void()> f_SetupJob;
	std::function<void()> f_InitializeJob;
	std::function<void()> f_PrepareCommandListJob;
	std::function<void()> f_ExecuteCommandListJob;
	std::function<void()> f_TerminateJob;

	bool m_showProbe = false;
	bool m_showLightHeatmap = false;
	bool m_saveScreenCapture = false;
	static bool m_drawBRDFTest = false;
}

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup()
{
	f_showProbe = [&]() { m_showProbe = !m_showProbe; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

	f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_C, true }, ButtonEvent{ EventLifeTime::OneShot, &f_saveScreenCapture });

	f_SetupJob = [&]()
	{
		DefaultGPUBuffers::Setup();
		GIDataLoader::Setup();

		LightCullingPass::Setup();
		GIResolvePass::Setup();
		GIResolveTestPass::Setup();
		BRDFLUTPass::Setup();
		SunShadowPass::Setup();
		OpaquePass::Setup();
		SSAOPass::Setup();
		LightPass::Setup();
		SkyPass::Setup();
		PreTAAPass::Setup();
		TransparentPass::Setup();
		//VolumetricFogPass::Setup();
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
		LightCullingPass::Initialize();
		GIResolvePass::Initialize();
		GIResolveTestPass::Initialize();
		BRDFLUTPass::Initialize();
		BRDFLUTPass::PrepareCommandList();
		BRDFLUTPass::ExecuteCommandList();
		SunShadowPass::Initialize();
		OpaquePass::Initialize();
		SSAOPass::Initialize();
		LightPass::Initialize();
		SkyPass::Initialize();
		PreTAAPass::Initialize();
		TransparentPass::Initialize();
		//VolumetricFogPass::Initialize();
		TAAPass::Initialize();
		PostTAAPass::Initialize();
		MotionBlurPass::Initialize();
		BillboardPass::Initialize();
		DebugPass::Initialize();
		FinalBlendPass::Initialize();

		BSDFTestPass::Initialize();
	};

	f_PrepareCommandListJob = [&]()
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();
		IResourceBinder* l_canvas;

		DefaultGPUBuffers::Upload();
		LightCullingPass::PrepareCommandList();
		GIResolvePass::PrepareCommandList();

		SunShadowPass::PrepareCommandList();
		OpaquePass::PrepareCommandList();
		SSAOPass::PrepareCommandList();
		LightPass::PrepareCommandList();

		if (l_renderingConfig.drawSky)
		{
			SkyPass::PrepareCommandList();
		}

		PreTAAPass::PrepareCommandList();
		TransparentPass::PrepareCommandList();
		//VolumetricFogPass::PrepareCommandList();

		l_canvas = TransparentPass::GetRPDC()->m_RenderTargetsResourceBinders[0];

		if (l_renderingConfig.useTAA)
		{
			TAAPass::PrepareCommandList(l_canvas);
			PostTAAPass::PrepareCommandList();
			l_canvas = PostTAAPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::PrepareCommandList(l_canvas);
			l_canvas = MotionBlurPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		if (m_drawBRDFTest)
		{
			BSDFTestPass::PrepareCommandList();
			l_canvas = BSDFTestPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		if (m_showLightHeatmap)
		{
			l_canvas = LightCullingPass::GetHeatMap();
		}

		if (m_showProbe)
		{
			GIResolveTestPass::PrepareCommandList();
			l_canvas = GIResolveTestPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		BillboardPass::PrepareCommandList();
		DebugPass::PrepareCommandList();

		FinalBlendPass::PrepareCommandList(l_canvas);
	};

	f_ExecuteCommandListJob = [&]()
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

		LightCullingPass::ExecuteCommandList();
		GIResolvePass::ExecuteCommandList();

		SunShadowPass::ExecuteCommandList();
		OpaquePass::ExecuteCommandList();
		SSAOPass::ExecuteCommandList();
		LightPass::ExecuteCommandList();

		if (l_renderingConfig.drawSky)
		{
			SkyPass::ExecuteCommandList();
		}

		PreTAAPass::ExecuteCommandList();
		TransparentPass::ExecuteCommandList();
		//VolumetricFogPass::ExecuteCommandList();

		if (l_renderingConfig.useTAA)
		{
			TAAPass::ExecuteCommandList();
			PostTAAPass::ExecuteCommandList();
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::ExecuteCommandList();
		}

		if (m_drawBRDFTest)
		{
			BSDFTestPass::ExecuteCommandList();
		}

		if (m_showProbe)
		{
			GIResolveTestPass::ExecuteCommandList();
		}

		BillboardPass::ExecuteCommandList();
		DebugPass::ExecuteCommandList();

		FinalBlendPass::ExecuteCommandList();

		if (m_saveScreenCapture)
		{
			auto l_textureData = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(FinalBlendPass::GetRPDC(), FinalBlendPass::GetRPDC()->m_RenderTargets[0]);
			auto l_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
			l_TDC->m_textureDataDesc = FinalBlendPass::GetRPDC()->m_RenderTargets[0]->m_textureDataDesc;
			l_TDC->m_textureData = l_textureData.data();
			g_pModuleManager->getFileSystem()->saveTexture("ScreenCapture", l_TDC);
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
		GIDataLoader::Terminate();
		BRDFLUTPass::Terminate();
		SunShadowPass::Terminate();
		OpaquePass::Terminate();
		SSAOPass::Terminate();
		LightPass::Terminate();
		SkyPass::Terminate();
		PreTAAPass::Terminate();
		TransparentPass::Terminate();
		//VolumetricFogPass::Terminate();
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

bool DefaultRenderingClient::PrepareCommandList()
{
	f_PrepareCommandListJob();

	return true;
}

bool DefaultRenderingClient::ExecuteCommandList()
{
	f_ExecuteCommandListJob();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	f_TerminateJob();

	return true;
}