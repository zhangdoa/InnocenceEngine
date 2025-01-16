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
#include "STL14.h"
#include "STL17.h"

namespace Inno
{
	class Memory
	{
	public:
		static void* Allocate(const std::size_t size);
		static void* Reallocate(void* const ptr, const std::size_t size);
		static void Deallocate(void* const ptr);
	
	private:
		bool Record(void* ptr, std::size_t size);

		bool Erase(void* ptr);

	private:
		std::shared_mutex m_Mutex;
		std::unordered_map<void*, std::size_t> m_Memo;
	};
}