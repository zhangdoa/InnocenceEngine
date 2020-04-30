#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VoxelizationPass
{
	bool Setup();
	bool Initialize();
	bool Render(bool visualize);
	bool Terminate();

	IResourceBinder* GetVoxelizationLuminanceVolume();
	IResourceBinder* GetVisualizationResult();
	IResourceBinder* GetVoxelizationCBuffer();
};
