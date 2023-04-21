#pragma once
#include "../Interface/IMemorySystem.h"

namespace Inno
{
	class MemorySystem : public IMemorySystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(MemorySystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void* allocate(size_t size) override;
		bool deallocate(void* ptr) override;
		void* reallocate(void* ptr, size_t size) override;
	};
}
