#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "BRDFLUTPass.h"
#include "OpaquePass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TAAPass.h"
#include "MotionBlurPass.h"
#include "FinalBlendPass.h"

bool DefaultRenderingClient::Setup()
{
	DefaultGPUBuffers::Setup();
	BRDFLUTPass::Setup();
	OpaquePass::Setup();
	SSAOPass::Setup();
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
	BRDFLUTPass::Initialize();
	BRDFLUTPass::PrepareCommandList();
	OpaquePass::Initialize();
	SSAOPass::Initialize();
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
	SSAOPass::PrepareCommandList();
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