#pragma once

#include <shared_mutex>
#include "../Interface/IService.h"
#include "../Common/Array.h"
#include "../Common/GPUDataStructure.h"
#include "../Component/GPUBufferComponent.h"

namespace Inno
{
	struct DrawCallServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		Inno::Array<RenderInstance> m_RenderInstanceVector;
		Inno::Array<MeshGeometry> m_MeshGeometryVector;
		Inno::Array<TransformConstantBuffer> m_TransformBufferVector;
		Inno::Array<MaterialConstantBuffer> m_MaterialCBVector;

		GPUBufferComponent* m_RenderInstanceBufferComp;
		GPUBufferComponent* m_TransformBufferComp;
		GPUBufferComponent* m_TransformPrevBufferComp;
		GPUBufferComponent* m_MaterialGPUBufferComp;
		GPUBufferComponent* m_MeshGeometryBufferComp;

		// Last mesh-residency epoch staged into the MeshGeometry table. UINT64_MAX
		// forces a stage on the first frame; thereafter the table + its upload only
		// run when AssetService reports a new epoch.
		uint64_t m_LastStagedResidencyEpoch = UINT64_MAX;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		void StageMeshGeometries();
		void CollectVisibleInstances();

		GPUBufferComponent* GetCurrentFrameTransformBuffer();
		GPUBufferComponent* GetPreviousFrameTransformBuffer();
	};
}
