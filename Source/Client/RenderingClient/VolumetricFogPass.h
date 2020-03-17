#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VolumetricFogPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	IResourceBinder* GetRayMarchingResult();
	IResourceBinder* GetFroxelVisualizationResult();
};
