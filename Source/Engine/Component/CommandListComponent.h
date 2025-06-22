#pragma once
#include "../Common/Object.h"

namespace Inno
{
	class CommandListComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 100; }
		static const char* GetTypeName() { return "CommandListComponent"; }
		
		uint64_t m_CommandList = 0;
		GPUEngineType m_Type = GPUEngineType::Graphics;
	};
}