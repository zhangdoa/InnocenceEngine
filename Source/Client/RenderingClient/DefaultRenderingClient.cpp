#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "OpaquePass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"
#include "MotionBlurPass.h"
#include "FinalBlendPass.h"

bool DefaultRenderingClient::Setup()
{
	DefaultGPUBuffers::Setup();
	OpaquePass::Setup();
	LightPass::Setup();
	SkyPass::Setup();
	PreTAAPass::Setup();
	TAAPass::Setup();
	MotionBlurPass::Setup();
	FinalBlendPass::Setup();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	DefaultGPUBuffers::Initialize();
	OpaquePass::Initialize();
	LightPass::Initialize();
	SkyPass::Initialize();
	PreTAAPass::Initialize();
	TAAPass::Initialize();
	MotionBlurPass::Initialize();
	FinalBlendPass::Initialize();

	return true;
}

bool DefaultRenderingClient::Render()
{
	DefaultGPUBuffers::Upload();
	OpaquePass::PrepareCommandList();
	LightPass::PrepareCommandList();
	SkyPass::PrepareCommandList();
	PreTAAPass::PrepareCommandList();
	TAAPass::PrepareCommandList();
	MotionBlurPass::PrepareCommandList(TAAPass::GetRPDC());
	FinalBlendPass::PrepareCommandList(MotionBlurPass::GetRPDC());

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}