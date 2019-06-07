#pragma once
#ifndef INNO_DECLSPEC_ALLOCATOR
#ifdef _MSC_VER
#define INNO_DECLSPEC_ALLOCATOR	__declspec(allocator)
#else
#define INNO_DECLSPEC_ALLOCATOR
#endif
#endif

#include <cstdio>

class IObjectPool
{
public:
	IObjectPool() = default;
	virtual ~IObjectPool() = default;

	virtual void* Spawn() = 0;
	virtual void Destroy(void* const ptr) = 0;
};

class InnoMemory
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	static void* Allocate(const std::size_t size);
	static void Deallocate(void* const ptr);

	static IObjectPool* CreateObjectPool(std::size_t objectSize, unsigned int poolCapability);
};
