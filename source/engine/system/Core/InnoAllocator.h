#pragma once
#include "InnoMemory.h"
#include <type_traits>

template <typename T>
class InnoAllocator
{
public:
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;
	using is_always_equal = std::true_type;

	constexpr InnoAllocator() noexcept
	{	// construct default allocator (do nothing)
	}

	constexpr InnoAllocator(const InnoAllocator&) noexcept = default;
	template<class _Other>
	constexpr InnoAllocator(const InnoAllocator<_Other>&) noexcept
	{	// construct from a related allocator (do nothing)
	}

	void deallocate(T * const _Ptr, const size_t _Count)
	{
		// deallocate object at _Ptr
		// !!!!!!Caution!!!!!!!No overflow prevent
		InnoMemory::Deallocate(_Ptr);
	}

	[[nodiscard]] INNO_DECLSPEC_ALLOCATOR T * allocate(const size_t _Count)
	{
		// allocate array of _Count elements
		// !!!!!!Caution!!!!!!!No overflow prevent
		return reinterpret_cast<T*>(InnoMemory::Allocate(sizeof(T) * _Count));
	}
};

template<class T,
	class _Other>
	[[nodiscard]] inline bool operator==(const InnoAllocator<T>&, const InnoAllocator<_Other>&) noexcept
{	// test for allocator equality
	return (true);
}

template<class T,
	class _Other>
	[[nodiscard]] inline bool operator!=(const InnoAllocator<T>&, const InnoAllocator<_Other>&) noexcept
{	// test for allocator inequality
	return (false);
}
