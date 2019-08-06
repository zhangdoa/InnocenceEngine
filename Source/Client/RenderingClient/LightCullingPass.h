#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace LightCullingPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	IResourceBinder* GetLightGrid();
	GPUBufferDataComponent* GetLightIndexList();
	IResourceBinder* GetHeatMap();
};
