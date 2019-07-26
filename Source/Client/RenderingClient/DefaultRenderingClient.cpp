#include "DefaultRenderingClient.h"
#include "OpaquePass.h"

bool DefaultRenderingClient::Setup()
{
	return true;
}

bool DefaultRenderingClient::Initialize()
{
	OpaquePass::Initialize();
	return true;
}

bool DefaultRenderingClient::Render()
{
	OpaquePass::PrepareCommandList();

	return true;
}

bool DefaultRenderingClient::Terminate()
{
	return true;
}