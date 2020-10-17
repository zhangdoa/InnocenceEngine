#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

using namespace Inno;
namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType
	{
		PerFrame,
		Mesh,
		Material,
		PointLight,
		SphereLight,
		CSM,
		ComputeDispatchParam,
		GI,
		Animation,
		Billboard
	};

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};