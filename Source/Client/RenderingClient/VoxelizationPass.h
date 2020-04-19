#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VoxelizationPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList(bool visualize);
	bool ExecuteCommandList(bool visualize);
	bool Terminate();

	IResourceBinder* GetVoxelizationLuminanceVolume();
	IResourceBinder* GetVisualizationResult();
	IResourceBinder* GetVoxelizationCBuffer();
};
