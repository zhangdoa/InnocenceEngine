#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "GIBakePass.h"
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
#include "FinalBlendPass.h"

#include "BRDFTestPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace DefaultRenderingClientNS
{
	std::function<void()> f_GIBake;
	bool m_needGIBake = false;
}

using namespace DefaultRenderingClientNS;

bool DefaultRenderingClient::Setup()
{
	f_GIBake = [&]() { m_needGIBake = true;	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_GIBake });

	DefaultGPUBuffers::Setup();
	GIBakePass::Setup();
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
	FinalBlendPass::Setup();

	BRDFTestPass::Setup();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	DefaultGPUBuffers::Initialize();
	GIBakePass::Initialize();
	BRDFLUTPass::Initialize();
	BRDFLUTPass::PrepareCommandList();
	SunShadowPass::Initialize();
	OpaquePass::Initialize();
	SSAOPass::Initialize();
	LightPass::Initialize();
	SkyPass::Initialize();
	PreTAAPass::Initialize();
	TAAPass::Initialize();
	PostTAAPass::Initialize();
	MotionBlurPass::Initialize();
	FinalBlendPass::Initialize();

	BRDFTestPass::Initialize();

	return true;
}

bool DefaultRenderingClient::Render()
{
	DefaultGPUBuffers::Upload();
	SunShadowPass::PrepareCommandList();
	OpaquePass::PrepareCommandList();
	SSAOPass::PrepareCommandList();
	LightPass::PrepareCommandList();
	SkyPass::PrepareCommandList();
	PreTAAPass::PrepareCommandList();
	TAAPass::PrepareCommandList();
	PostTAAPass::PrepareCommandList();
	MotionBlurPass::PrepareCommandList(PostTAAPass::GetRPDC());
	//BRDFTestPass::PrepareCommandList();
	FinalBlendPass::PrepareCommandList(MotionBlurPass::GetRPDC());

	SunShadowPass::ExecuteCommandList();
	OpaquePass::ExecuteCommandList();
	SSAOPass::ExecuteCommandList();
	LightPass::ExecuteCommandList();
	SkyPass::ExecuteCommandList();
	PreTAAPass::ExecuteCommandList();
	TAAPass::ExecuteCommandList();
	PostTAAPass::ExecuteCommandList();
	MotionBlurPass::ExecuteCommandList();
	//BRDFTestPass::ExecuteCommandList();

	FinalBlendPass::ExecuteCommandList();

	if (m_needGIBake)
	{
		GIBakePass::Bake();
		m_needGIBake = false;
	}

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}