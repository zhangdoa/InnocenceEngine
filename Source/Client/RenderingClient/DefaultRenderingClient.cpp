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

	std::function<void()> f_SetupTask;
	std::function<void()> f_InitializeTask;
	std::function<void()> f_RenderTask;
	std::function<void()> f_TerminateTask;

	bool m_showProbe = false;
	bool m_showLightHeatmap = false;
}

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup()
{
	f_showProbe = [&]() { m_showProbe = !m_showProbe; };
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

	f_SetupTask = [&]()
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
		TAAPass::Setup();
		PostTAAPass::Setup();
		MotionBlurPass::Setup();
		BillboardPass::Setup();
		DebugPass::Setup();
		FinalBlendPass::Setup();

		BSDFTestPass::Setup();
	};

	f_InitializeTask = [&]()
	{
		DefaultGPUBuffers::Initialize();
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
		TAAPass::Initialize();
		PostTAAPass::Initialize();
		MotionBlurPass::Initialize();
		BillboardPass::Initialize();
		DebugPass::Initialize();
		FinalBlendPass::Initialize();

		BSDFTestPass::Initialize();
	};

	f_RenderTask = [&]()
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

		static bool l_drawBRDFTest = false;
		if (l_drawBRDFTest)
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

		if (l_renderingConfig.useTAA)
		{
			TAAPass::ExecuteCommandList();
			PostTAAPass::ExecuteCommandList();
		}

		if (l_renderingConfig.useMotionBlur)
		{
			MotionBlurPass::ExecuteCommandList();
		}

		if (l_drawBRDFTest)
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
	};

	f_TerminateTask = [&]()
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
		TAAPass::Terminate();
		PostTAAPass::Terminate();
		MotionBlurPass::Terminate();
		BillboardPass::Terminate();
		DebugPass::Terminate();
		FinalBlendPass::Terminate();

		BSDFTestPass::Terminate();
	};

	auto l_DefaultRenderingClientSetupTask = g_pModuleManager->getTaskSystem()->submit("DefaultRenderingClientSetupTask", 2, nullptr, f_SetupTask);
	l_DefaultRenderingClientSetupTask->Wait();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	auto l_DefaultRenderingClientInitializeTask = g_pModuleManager->getTaskSystem()->submit("DefaultRenderingClientSetupTask", 2, nullptr, f_InitializeTask);
	l_DefaultRenderingClientInitializeTask->Wait();

	return true;
}

bool DefaultRenderingClient::Render()
{
	f_RenderTask();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	f_TerminateTask();

	return true;
}