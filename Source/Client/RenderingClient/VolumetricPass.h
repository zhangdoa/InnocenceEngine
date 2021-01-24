#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace VolumetricPass
{
	bool Setup();
	bool Initialize();
	bool Render(bool visualize);
	bool Terminate();

	GPUResourceComponent* GetRayMarchingResult();
	GPUResourceComponent* GetVisualizationResult();
};
