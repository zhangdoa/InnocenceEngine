#pragma once
#include "common/stdafx.h"
#include "interface/IMemorySystem.h"

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

	pointer allocate(size_type n, innoAllocator<void>::const_pointer hint = 0)
	{
		return g_pMemorySystem->spawn<T>();
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

template<class _Ty, class _Ax = innoAllocator<_Ty> >
class innoList : public std::list<_Ty, _Ax>
{
};

