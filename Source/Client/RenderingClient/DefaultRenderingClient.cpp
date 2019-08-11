#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "LightCullingPass.h"
#include "GIBakePass.h"
#include "GIResolvePass.h"
#include "BRDFLUTPass.h"
#include "SunShadowPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BRDFTestPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace DefaultRenderingClientNS
{
	std::function<void()> f_GIBake;
	std::function<void()> f_showLightHeatmap;
	std::function<void()> f_SetupTask;
	std::function<void()> f_InitializeTask;
	std::function<void()> f_RenderTask;
	std::function<void()> f_TerminateTask;
	bool m_needGIBake = false;
	bool m_showLightHeatmap = false;
}

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup()
{
	f_GIBake = [&]() { m_needGIBake = true;	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_GIBake });

	f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap;	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

	f_SetupTask = [&]()
	{
		DefaultGPUBuffers::Setup();
		LightCullingPass::Setup();
		GIBakePass::Setup();
		//GIResolvePass::Setup();
		BRDFLUTPass::Setup();
		SunShadowPass::Setup();
		OpaquePass::Setup();
		SSAOPass::Setup();
		LightPass::Setup();
		SkyPass::Setup();
		PreTAAPass::Setup();
		TAAPass::Setup();
		PostTAAPass::Setup();
		MotionBlurPass::Setup();
		BillboardPass::Setup();
		DebugPass::Setup();
		FinalBlendPass::Setup();

		BRDFTestPass::Setup();
	};

	f_InitializeTask = [&]()
	{
		DefaultGPUBuffers::Initialize();
		LightCullingPass::Initialize();
		GIBakePass::Initialize();
		//GIResolvePass::Initialize();
		BRDFLUTPass::Initialize();
		BRDFLUTPass::PrepareCommandList();
		BRDFLUTPass::ExecuteCommandList();
		SunShadowPass::Initialize();
		OpaquePass::Initialize();
		SSAOPass::Initialize();
		LightPass::Initialize();
		SkyPass::Initialize();
		PreTAAPass::Initialize();
		TAAPass::Initialize();
		PostTAAPass::Initialize();
		MotionBlurPass::Initialize();
		BillboardPass::Initialize();
		DebugPass::Initialize();
		FinalBlendPass::Initialize();

		BRDFTestPass::Initialize();
	};

	f_RenderTask = [&]()
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();
		IResourceBinder* l_canvas;

		DefaultGPUBuffers::Upload();
		//LightCullingPass::PrepareCommandList();
		//GIResolvePass::PrepareCommandList();

		SunShadowPass::PrepareCommandList();
		OpaquePass::PrepareCommandList();
		SSAOPass::PrepareCommandList();
		LightPass::PrepareCommandList();

		if (l_renderingConfig.drawSky)
		{
			SkyPass::PrepareCommandList();
		}

		PreTAAPass::PrepareCommandList();
		l_canvas = PreTAAPass::GetRPDC()->m_RenderTargetsResourceBinders[0];

		if (l_renderingConfig.useTAA)
		{
			TAAPass::PrepareCommandList();
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
			BRDFTestPass::PrepareCommandList();
			l_canvas = BRDFTestPass::GetRPDC()->m_RenderTargetsResourceBinders[0];
		}

		if (m_showLightHeatmap)
		{
			l_canvas = LightCullingPass::GetHeatMap();
		}

		BillboardPass::PrepareCommandList();
		DebugPass::PrepareCommandList();

		FinalBlendPass::PrepareCommandList(l_canvas);

		//LightCullingPass::ExecuteCommandList();
		//GIResolvePass::ExecuteCommandList();

		SunShadowPass::ExecuteCommandList();
		OpaquePass::ExecuteCommandList();
		SSAOPass::ExecuteCommandList();
		LightPass::ExecuteCommandList();
		SkyPass::ExecuteCommandList();
		PreTAAPass::ExecuteCommandList();
		TAAPass::ExecuteCommandList();
		PostTAAPass::ExecuteCommandList();
		MotionBlurPass::ExecuteCommandList();
		BRDFTestPass::ExecuteCommandList();

		BillboardPass::ExecuteCommandList();
		DebugPass::ExecuteCommandList();

		FinalBlendPass::ExecuteCommandList();

		if (m_needGIBake)
		{
			GIBakePass::Bake();
			m_needGIBake = false;
		}
	};

	f_TerminateTask = [&]()
	{
		DefaultGPUBuffers::Terminate();
		LightCullingPass::Terminate();
		//GIResolvePass::Terminate();
		GIBakePass::Terminate();
		BRDFLUTPass::Terminate();
		SunShadowPass::Terminate();
		OpaquePass::Terminate();
		SSAOPass::Terminate();
		LightPass::Terminate();
		SkyPass::Terminate();
		PreTAAPass::Terminate();
		TAAPass::Terminate();
		PostTAAPass::Terminate();
		MotionBlurPass::Terminate();
		BillboardPass::Terminate();
		DebugPass::Terminate();
		FinalBlendPass::Terminate();

		BRDFTestPass::Terminate();
	};

	f_SetupTask();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	f_InitializeTask();

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