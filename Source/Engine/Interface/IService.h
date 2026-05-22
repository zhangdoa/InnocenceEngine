#pragma once
#include "../Common/Array.h"
#include "../Common/ClassTemplate.h"
#include "../Common/Object.h"
#include <vector>
#include <typeindex>

namespace Inno
{
	class IServiceConfig
	{
	};

	class IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IService);

		virtual bool Setup(IServiceConfig* systemConfig = nullptr) = 0;
		virtual bool Initialize() = 0;
		virtual bool Update() { return true; };
		virtual bool Terminate() = 0;
		virtual ObjectStatus GetStatus() = 0;
		
		// Dependency resolution support
		virtual Inno::Array<std::type_index> GetDependencies() { return {}; }
	};
}