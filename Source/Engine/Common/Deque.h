#pragma once
#include "Array.h"
#include "Allocator.h"

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Inno
{
	// Chunked, pointer-stable, Allocator-backed sequence. Existing element
	// addresses never move across emplace_back — full chunks stay put;
	// only m_Chunks (array of chunk pointers) reallocates. AssetService's
	// GetMeshAsset returns stable T* while Allocate concurrently appends;
	// that is the contract std::deque previously provided.
	template <class T, size_t kChunkSize = 64>
	class Deque
	{
		static_assert((kChunkSize & (kChunkSize - 1)) == 0, "kChunkSize must be a power of two");

	public:
		using value_type = T;
		using size_type  = size_t;

		Deque() = default;

		Deque(const Deque& rhs)
		{
			m_Chunks.reserve(rhs.m_Chunks.size());
			for (size_type i = 0; i < rhs.m_Size; ++i) emplace_back(rhs[i]);
		}

		Deque& operator=(const Deque& rhs)
		{
			if (this != &rhs)
			{
				clear();
				m_Chunks.reserve(rhs.m_Chunks.size());
				for (size_type i = 0; i < rhs.m_Size; ++i) emplace_back(rhs[i]);
			}
			return *this;
		}

		Deque(Deque&& rhs) noexcept
			: m_Chunks(std::move(rhs.m_Chunks)), m_Size(rhs.m_Size)
		{
			rhs.m_Size = 0;
		}

		Deque& operator=(Deque&& rhs) noexcept
		{
			if (this != &rhs)
			{
				clear();
				m_Chunks = std::move(rhs.m_Chunks);
				m_Size = rhs.m_Size;
				rhs.m_Size = 0;
			}
			return *this;
		}

		~Deque() { clear(); }

		size_type size()  const noexcept { return m_Size; }
		bool      empty() const noexcept { return m_Size == 0; }

		template <class... Args>
		T& emplace_back(Args&&... args)
		{
			const size_type slotInChunk = m_Size & (kChunkSize - 1);
			if (slotInChunk == 0)
			{
				T* fresh = m_Alloc.allocate(kChunkSize);
				m_Chunks.push_back(fresh);
			}
			T* chunk = m_Chunks[m_Size / kChunkSize];
			T* slot = chunk + slotInChunk;
			::new (static_cast<void*>(slot)) T(std::forward<Args>(args)...);
			++m_Size;
			return *slot;
		}

		T& operator[](size_type i)
		{
			assert(i < m_Size);
			return m_Chunks[i / kChunkSize][i & (kChunkSize - 1)];
		}

		const T& operator[](size_type i) const
		{
			assert(i < m_Size);
			return m_Chunks[i / kChunkSize][i & (kChunkSize - 1)];
		}

		void clear()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (size_type i = 0; i < m_Size; ++i) (*this)[i].~T();
			}
			for (size_type c = 0; c < m_Chunks.size(); ++c)
				m_Alloc.deallocate(m_Chunks[c], kChunkSize);
			m_Chunks.clear();
			m_Size = 0;
		}

	private:
		Allocator<T>         m_Alloc{};
		Array<T*>            m_Chunks;
		size_type            m_Size = 0;
	};
}
