#pragma once
#include "../../Engine/RenderingServer/IGraphicsService.h"

using namespace Inno;
namespace VolumetricPass
{
	bool Setup();
	bool Initialize();
	bool ExecuteCommands(bool visualize);
	bool Terminate();

	GPUResourceComponent* GetRayMarchingResult();
	GPUResourceComponent* GetVisualizationResult();
};
