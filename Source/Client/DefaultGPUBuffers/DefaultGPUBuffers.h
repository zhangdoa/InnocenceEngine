#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

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
		Billboard
	};

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};