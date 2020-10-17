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

namespace Inno
{
	class InnoMemory
	{
	public:
		static void* Allocate(const std::size_t size);
		static void* Reallocate(void* const ptr, const std::size_t size);
		static void Deallocate(void* const ptr);
	};
}