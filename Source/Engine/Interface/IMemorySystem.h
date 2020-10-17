#pragma once
#include "ISystem.h"

namespace Inno
{
	class IMemorySystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IMemorySystem);

		virtual void* allocate(size_t size) = 0;
		virtual bool deallocate(void* ptr) = 0;
		virtual void* reallocate(void* ptr, size_t size) = 0;
	};
}