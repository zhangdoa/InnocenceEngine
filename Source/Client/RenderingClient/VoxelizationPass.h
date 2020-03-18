#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VoxelizationPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	IResourceBinder* GetVisualizationResult();
};
