#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "OpaquePass.h"
#include "LightPass.h"
#include "FinalBlendPass.h"

bool DefaultRenderingClient::Setup()
{
	DefaultGPUBuffers::Setup();
	OpaquePass::Setup();
	LightPass::Setup();
	FinalBlendPass::Setup();

	return true;
}

bool DefaultRenderingClient::Initialize()
{
	DefaultGPUBuffers::Initialize();
	OpaquePass::Initialize();
	LightPass::Initialize();
	FinalBlendPass::Initialize();

	return true;
}

bool DefaultRenderingClient::Render()
{
	DefaultGPUBuffers::Upload();
	OpaquePass::PrepareCommandList();
	LightPass::PrepareCommandList();
	FinalBlendPass::PrepareCommandList();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}