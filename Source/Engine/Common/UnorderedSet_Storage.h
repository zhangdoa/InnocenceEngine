#pragma once
#include "Allocator.h"

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Inno
{
	// Storage + open-addressing helpers shared by UnorderedSet. Slot states:
	// 0 = Empty, 1 = Occupied (matches UnorderedSet_Iterator::kOccupied),
	// 2 = Tombstone. UnorderedSet derives and adds public set semantics on top.
	template <class T, class Hash, class KeyEqual>
	class UnorderedSetStorage
	{
	public:
		enum SlotState : uint8_t { Empty = 0, Occupied = 1, Tombstone = 2 };
		using value_type = T;
		using size_type  = size_t;

	protected:
		UnorderedSetStorage() = default;
		~UnorderedSetStorage() { destroy_and_free(); }

		void destroy_and_free()
		{
			if (!m_state) return;
			clear();
			m_dataAlloc.deallocate(m_data, m_capacity);
			m_stateAlloc.deallocate(m_state, m_capacity);
			m_data = nullptr;
			m_state = nullptr;
			m_capacity = 0;
		}

		void clear()
		{
			if (!m_state) return;
			for (size_type i = 0; i < m_capacity; ++i)
			{
				if (m_state[i] == Occupied)
					if constexpr (!std::is_trivially_destructible_v<value_type>) m_data[i].~value_type();
				m_state[i] = Empty;
			}
			m_size = 0;
			m_tombstones = 0;
		}

		size_type mask() const noexcept { return m_capacity - 1; }

		static size_type round_up_pow2(size_type n)
		{
			if (n <= 1) return 1;
			size_type p = 1;
			while (p < n) p <<= 1;
			return p;
		}

		size_type first_occupied(size_type start) const
		{
			while (start < m_capacity && m_state[start] != Occupied) ++start;
			return start;
		}

		bool locate(const T& key, size_type& outIdx) const
		{
			assert(m_capacity > 0);
			const size_type m = mask();
			const size_type start = Hash{}(key) & m;
			size_type i = start;
			size_type firstTombstone = static_cast<size_type>(-1);
			KeyEqual eq{};
			do
			{
				if (m_state[i] == Empty)
				{
					outIdx = firstTombstone == static_cast<size_type>(-1) ? i : firstTombstone;
					return false;
				}
				if (m_state[i] == Occupied && eq(m_data[i], key)) { outIdx = i; return true; }
				if (m_state[i] == Tombstone && firstTombstone == static_cast<size_type>(-1)) firstTombstone = i;
				i = (i + 1) & m;
			} while (i != start);
			outIdx = firstTombstone == static_cast<size_type>(-1) ? start : firstTombstone;
			return false;
		}

		template <class V>
		void insert_into_empty_slots(V&& value)
		{
			const size_type m = mask();
			size_type i = Hash{}(value) & m;
			while (m_state[i] != Empty) i = (i + 1) & m;
			::new (static_cast<void*>(m_data + i)) value_type(std::forward<V>(value));
			m_state[i] = Occupied;
			++m_size;
		}

		void ensure_capacity_for_one_more()
		{
			if (m_capacity == 0 || (m_size + m_tombstones + 1) * 4 >= m_capacity * 3)
				grow_to(m_capacity == 0 ? 8 : m_capacity * 2);
		}

		void grow_to(size_type newCap)
		{
			if (newCap <= m_capacity && m_tombstones == 0) return;
			value_type* oldData  = m_data;
			uint8_t*    oldState = m_state;
			size_type   oldCap   = m_capacity;
			m_data  = m_dataAlloc.allocate(newCap);
			m_state = m_stateAlloc.allocate(newCap);
			for (size_type i = 0; i < newCap; ++i) m_state[i] = Empty;
			m_capacity = newCap;
			m_size = 0;
			m_tombstones = 0;
			if (oldState)
			{
				for (size_type i = 0; i < oldCap; ++i)
				{
					if (oldState[i] == Occupied)
					{
						insert_into_empty_slots(std::move(oldData[i]));
						if constexpr (!std::is_trivially_destructible_v<value_type>) oldData[i].~value_type();
					}
				}
				m_dataAlloc.deallocate(oldData, oldCap);
				m_stateAlloc.deallocate(oldState, oldCap);
			}
		}

		Allocator<value_type> m_dataAlloc{};
		Allocator<uint8_t>    m_stateAlloc{};
		value_type* m_data = nullptr;
		uint8_t*    m_state = nullptr;
		size_type   m_capacity = 0;
		size_type   m_size = 0;
		size_type   m_tombstones = 0;
	};
}
