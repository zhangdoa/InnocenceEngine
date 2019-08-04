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

bool DefaultRenderingClient::Setup()
{
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

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}