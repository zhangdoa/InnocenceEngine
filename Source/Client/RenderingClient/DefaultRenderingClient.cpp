#include "DefaultRenderingClient.h"
#include "DefaultGPUBuffers.h"
#include "OpaquePass.h"
#include "LightPass.h"

bool DefaultRenderingClient::Setup()
{
	return true;
}

bool DefaultRenderingClient::Initialize()
{
	DefaultGPUBuffers::Initialize();
	OpaquePass::Initialize();
	LightPass::Initialize();

	return true;
}

bool DefaultRenderingClient::Render()
{
	DefaultGPUBuffers::Upload();
	OpaquePass::PrepareCommandList();
	LightPass::PrepareCommandList();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}