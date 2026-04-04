#pragma once
#include "../Common/GPUMeshResource.h"
#include "../Common/Object.h"

namespace Inno
{
	struct MeshComponent
	{
		static uint32_t GetTypeID() { return 6; };
		static const char* GetTypeName() { return "MeshComponent"; };

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		ObjectName   m_InstanceName = "";

		GPUMeshResourceHandle m_GPUResource;
	};
}
