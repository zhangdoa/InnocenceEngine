#pragma once
#include "../Common/AssetHandle.h"
#include "../Common/Object.h"

namespace Inno
{
	struct MaterialComponent
	{
		static uint32_t GetTypeID() { return 7; };
		static const char* GetTypeName() { return "MaterialComponent"; };

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		ObjectName   m_InstanceName = "";

		MaterialAssetHandle m_Asset;
	};
}
