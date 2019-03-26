#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE IMemorySystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IMemorySystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void* allocateMemoryPool(size_t objectSize, unsigned int poolCapability) = 0;
	virtual void* allocateRawMemory(size_t size) = 0;

	virtual void* spawnObject(void* memoryPool, size_t objectSize) = 0;
	virtual bool destroyObject(void* memoryPool, size_t objectSize, void* object) = 0;
};