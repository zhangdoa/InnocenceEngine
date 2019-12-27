#pragma once
#ifndef INNO_DECLSPEC_ALLOCATOR
#ifdef _MSC_VER
#define INNO_DECLSPEC_ALLOCATOR	__declspec(allocator)
#else
#define INNO_DECLSPEC_ALLOCATOR
#endif
#endif

#include <cstdio>
#include <cstdint>

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
	static void* Allocate(const std::size_t size);
	static void* Reallocate(void* const ptr, const std::size_t size);
	static void Deallocate(void* const ptr);

	template <typename T>
	static T* Spawn(IObjectPool* objectPool)
	{
		return new(objectPool->Spawn()) T();
	};

	template <typename T>
	static void Destroy(IObjectPool* objectPool, T* const ptr)
	{
		return objectPool->Destroy(ptr);
	}

private:
	static IObjectPool* CreateObjectPool(std::size_t objectSize, uint32_t poolCapability);

public:
	template <typename T>
	static IObjectPool* CreateObjectPool(uint32_t poolCapability)
	{
		return CreateObjectPool(sizeof(T), poolCapability);
	}

	static bool ClearObjectPool(IObjectPool* objectPool);
	static bool DestroyObjectPool(IObjectPool* objectPool);
};