#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType
	{
		PerFrame,
		SunShadowPassMesh,
		OpaquePassMesh,
		OpaquePassMaterial,
		TransparentPassMesh,
		TransparentPassMaterial,
		VolumetricFogPassMesh,
		VolumetricFogPassMaterial,
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