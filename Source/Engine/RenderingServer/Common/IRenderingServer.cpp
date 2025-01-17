#include "../IRenderingServer.h"

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Common/ThreadSafeQueue.h"

#include "../../Engine.h"
using namespace Inno;

void IRenderingServer::TransferDataToGPU()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshComponent* l_Mesh;
		m_uninitializedMeshes.tryPop(l_Mesh);

		if (l_Mesh)
		{
			auto l_result = InitializeMeshComponent(l_Mesh);
		}
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialComponent* l_Material;
		m_uninitializedMaterials.tryPop(l_Material);

		if (l_Material)
		{
			auto l_result = InitializeMaterialComponent(l_Material);
		}
	}
}

void IRenderingServer::InitializeMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMeshes.push(rhs);
	}
	else
	{
		auto l_MeshComponentInitializeTask = g_Engine->Get<TaskScheduler>()->Submit("MeshComponentInitializeTask", 2,
			[=]() { InitializeMeshComponent(rhs); });
		l_MeshComponentInitializeTask->Wait();
	}
}

void IRenderingServer::InitializeMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMaterials.push(rhs);
	}
	else
	{
		auto l_MaterialComponentInitializeTask = g_Engine->Get<TaskScheduler>()->Submit("MaterialComponentInitializeTask", 2,
			[=]() { InitializeMaterialComponent(rhs); });
		l_MaterialComponentInitializeTask->Wait();
	}
}
