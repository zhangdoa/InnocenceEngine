#pragma once
#include "../Common/ClassTemplate.h"
#include "../Common/Object.h"
#include <vector>
#include <typeindex>

namespace Inno
{
	class ISystemConfig
	{
	};

	class ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ISystem);

		virtual bool Setup(ISystemConfig* systemConfig = nullptr) = 0;
		virtual bool Initialize() = 0;
		virtual bool Update() { return true; };
		virtual bool Terminate() = 0;
		virtual ObjectStatus GetStatus() = 0;
		
		// Dependency resolution support
		virtual std::vector<std::type_index> GetDependencies() { return {}; }
	};
}