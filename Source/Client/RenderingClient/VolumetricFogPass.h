#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VolumetricFogPass
{
	bool Setup();
	bool Initialize();
	bool Render(bool visualize);
	bool Terminate();

	IResourceBinder* GetRayMarchingResult();
	IResourceBinder* GetFroxelVisualizationResult();
};
