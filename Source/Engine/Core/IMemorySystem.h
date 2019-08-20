#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

class IMemorySystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IMemorySystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void* allocate(size_t size) = 0;
	virtual bool deallocate(void* ptr) = 0;
};