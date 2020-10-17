#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace GIResolvePass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();
	bool InitializeGPUBuffers();
	bool DeleteGPUBuffers();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
	IResourceBinder* GetProbeVolume();
	IResourceBinder* GetIrradianceVolume();
};
