#pragma once
#include "../Interface/IMemorySystem.h"

class InnoMemorySystem : public IMemorySystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoMemorySystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void* allocate(size_t size) override;
	bool deallocate(void* ptr) override;
	void* reallocate(void* ptr, size_t size) override;
};
