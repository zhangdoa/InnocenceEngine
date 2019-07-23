#pragma once
#include "IMemorySystem.h"

class InnoMemorySystem : public IMemorySystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoMemorySystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void* allocateMemoryPool(size_t objectSize, unsigned int poolCapability) override;
	void* allocateRawMemory(size_t size) override;
	bool deallocateRawMemory(void* ptr) override;

	void* spawnObject(void* memoryPool, size_t objectSize) override;
	bool destroyObject(void* memoryPool, size_t objectSize, void* object) override;
};
