#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace VolumetricPass
{
	bool Setup();
	bool Initialize();
	bool Render(bool visualize);
	bool Terminate();

	IResourceBinder* GetRayMarchingResult();
	IResourceBinder* GetVisualizationResult();
};
