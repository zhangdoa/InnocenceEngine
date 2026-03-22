#pragma once
#include "../Common/Object.h"
#include "../Common/GraphicsPrimitive.h"

namespace Inno
{
	struct CommandListComponent
	{
		ObjectStatus  m_ObjectStatus = ObjectStatus::Invalid;
		ObjectName    m_InstanceName = "";

		uint64_t      m_CommandList  = 0;
		GPUEngineType m_Type         = GPUEngineType::Graphics;
	};
}
