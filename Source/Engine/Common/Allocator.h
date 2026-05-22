#pragma once
#include "Memory.h"
#include <cstddef>
#include <new>
#include <type_traits>

namespace Inno
{
	// Routes through Inno::Memory (std::malloc/std::realloc/std::free)
	// for engine-wide alloc bookkeeping. Alignment honoured up to
	// alignof(std::max_align_t) (16 on Win64) — sufficient for every
	// engine type today; SIMD-aligned T would need an aligned allocator.
	template <typename T>
	class Allocator
	{
	public:
		using value_type = T;
		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal = std::true_type;

		constexpr Allocator() noexcept = default;
		constexpr Allocator(const Allocator&) noexcept = default;
		template<class _Other>
		constexpr Allocator(const Allocator<_Other>&) noexcept {}

		void deallocate(T* const _Ptr, const size_t /*_Count*/)
		{
			Memory::Deallocate(_Ptr);
		}

		[[nodiscard]] INNO_DECLSPEC_ALLOCATOR T* allocate(const size_t _Count)
		{
			if (_Count > (static_cast<size_t>(-1) / sizeof(T)))
				throw std::bad_alloc{};
			return reinterpret_cast<T*>(Memory::Allocate(sizeof(T) * _Count));
		}
	};

	template<class T, class _Other>
	[[nodiscard]] inline bool operator==(const Allocator<T>&, const Allocator<_Other>&) noexcept
	{
		return true;
	}

	template<class T, class _Other>
	[[nodiscard]] inline bool operator!=(const Allocator<T>&, const Allocator<_Other>&) noexcept
	{
		return false;
	}
}
