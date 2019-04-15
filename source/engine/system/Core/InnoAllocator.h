#pragma once
#include "../../common/stl14.h"

#ifndef INNO_DECLSPEC_ALLOCATOR
#ifdef __clang__
#define INNO_DECLSPEC_ALLOCATOR
#else /* ^^^ Clang ^^^ // vvv non-Clang vvv */
#define INNO_DECLSPEC_ALLOCATOR	__declspec(allocator)
#endif /* ^^^ non-Clang ^^^ */

#endif /* _DECLSPEC_ALLOCATOR */

namespace innoHeapAllocator
{
	void deallocate(void * const ptr);

	void* allocate(const size_t size);
};

template <typename T>
class innoAllocator
{
public:
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;
	using is_always_equal = std::true_type;

	constexpr innoAllocator() noexcept
	{	// construct default allocator (do nothing)
	}

	constexpr innoAllocator(const innoAllocator&) noexcept = default;
	template<class _Other>
	constexpr innoAllocator(const innoAllocator<_Other>&) noexcept
	{	// construct from a related allocator (do nothing)
	}

	void deallocate(T * const _Ptr, const size_t _Count)
	{
		// deallocate object at _Ptr
		// !!!!!!Caution!!!!!!!No overflow prevent
		innoHeapAllocator::deallocate(_Ptr);
	}

	[[nodiscard]] INNO_DECLSPEC_ALLOCATOR T * allocate(const size_t _Count)
	{
		// allocate array of _Count elements
		// !!!!!!Caution!!!!!!!No overflow prevent
		return reinterpret_cast<T*>(innoHeapAllocator::allocate(sizeof(T) * _Count));
	}
};

template<class _Ty,
	class _Other>
	[[nodiscard]] inline bool operator==(const innoAllocator<_Ty>&, const innoAllocator<_Other>&) noexcept
{	// test for allocator equality
	return (true);
}

template<class _Ty,
	class _Other>
	[[nodiscard]] inline bool operator!=(const innoAllocator<_Ty>&, const innoAllocator<_Other>&) noexcept
{	// test for allocator inequality
	return (false);
}