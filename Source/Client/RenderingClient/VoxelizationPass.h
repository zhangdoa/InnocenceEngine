#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VoxelizationPass
{
	bool Setup();
	bool Initialize();
	bool Render(bool visualize, uint32_t multiBounceCount, bool screenFeedback);
	bool Terminate();

	IResourceBinder* GetVoxelizationLuminanceVolume();
	IResourceBinder* GetVisualizationResult();
	IResourceBinder* GetVoxelizationCBuffer();
};
