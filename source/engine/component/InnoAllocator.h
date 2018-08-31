#pragma once
#include "../common/stdafx.h"
#include "../interface/IMemorySystem.h"

extern IMemorySystem* g_pMemorySystem;

template <typename T>
class innoAllocator
{
public:

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T value_type;

	innoAllocator() {}
	~innoAllocator() {}

	template <class U> struct rebind { typedef innoAllocator<U> other; };
	template <class U> innoAllocator(const innoAllocator<U>&) {}

	pointer allocate(size_type n, const_pointer hint = 0)
	{
		return g_pMemorySystem->spawn<T>(n);
	}

	void deallocate(pointer p, unsigned n)
	{
		return g_pMemorySystem->destroy(p);
	}

	bool operator==(const innoAllocator &rhs)
	{
		return true;
	}

	bool operator!=(const innoAllocator &rhs)
	{
		return !operator==(rhs);
	}
};

