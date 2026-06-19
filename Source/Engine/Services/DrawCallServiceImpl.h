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

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateDrawCalls();
		void StageMeshGeometries();
		void CollectVisibleInstances();

		GPUBufferComponent* GetCurrentFrameTransformBuffer();
		GPUBufferComponent* GetPreviousFrameTransformBuffer();
	};
}
