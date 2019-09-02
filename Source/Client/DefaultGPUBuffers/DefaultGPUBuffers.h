#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType
	{
		MainCamera,
		SunShadowPassMesh,
		OpaquePassMesh,
		OpaquePassMaterial,
		TransparentPassMesh,
		TransparentPassMaterial,
		Sun,
		PointLight,
		SphereLight,
		CSM,
		Sky,
		Compute,
		GICamera,
		GISky,
		Billboard
	};

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};