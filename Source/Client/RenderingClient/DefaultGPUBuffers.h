#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType { Camera, Mesh, Material, Sun, PointLight, SphereLight, CSM, Sky, Compute, SH9, Billboard, Debug };

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};