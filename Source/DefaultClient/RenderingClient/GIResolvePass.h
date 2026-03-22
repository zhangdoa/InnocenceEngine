#pragma once
#include "../../Engine/Services/IGraphicsService.h"

using namespace Inno;
namespace GIResolvePass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();
	bool InitializeGPUBuffers();
	bool DeleteGPUBuffers();

	RenderPassComponent* GetRenderPassComp();
	ShaderProgramComponent* GetSPC();
	GPUResourceComponent* GetProbeVolume();
	GPUResourceComponent* GetIrradianceVolume();
};
