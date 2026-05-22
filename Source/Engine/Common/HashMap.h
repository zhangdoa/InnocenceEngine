#pragma once
#include "Allocator.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace Inno
{
	// Engine-native hash table. Open addressing + linear probing.
	// Power-of-2 capacity (mask-modulo). Rehash at load factor 0.75.
	// Default hash is std::hash<Key>; override via Hash template parameter.
	// Not thread-safe; caller serialises (ThreadSafeUnorderedMap wraps this).
	template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
	class HashMap
	{
		enum SlotState : uint8_t { Empty = 0, Occupied = 1, Tombstone = 2 };

	public:
		using key_type    = Key;
		using mapped_type = T;
		using value_type  = T;
		using size_type   = size_t;

		HashMap() = default;

		explicit HashMap(size_type initialCapacity)
		{
			reserve(initialCapacity);
		}

		HashMap(const HashMap& rhs)
		{
			if (rhs.m_size == 0) return;
			grow_to(rhs.m_capacity);
			for (size_type i = 0; i < rhs.m_capacity; ++i)
			{
				if (rhs.m_state[i] == Occupied)
					insert_into_empty_slots(rhs.m_keys[i], rhs.m_values[i]);
			}
		}

		HashMap& operator=(const HashMap& rhs)
		{
			if (this != &rhs)
			{
				clear();
				if (rhs.m_size == 0) return *this;
				if (m_capacity < rhs.m_capacity) grow_to(rhs.m_capacity);
				for (size_type i = 0; i < rhs.m_capacity; ++i)
				{
					if (rhs.m_state[i] == Occupied)
						insert_into_empty_slots(rhs.m_keys[i], rhs.m_values[i]);
				}
			}
			return *this;
		}

		HashMap(HashMap&& rhs) noexcept
			: m_keys(rhs.m_keys), m_values(rhs.m_values), m_state(rhs.m_state)
			, m_capacity(rhs.m_capacity), m_size(rhs.m_size), m_tombstones(rhs.m_tombstones)
		{
			rhs.m_keys = nullptr;
			rhs.m_values = nullptr;
			rhs.m_state = nullptr;
			rhs.m_capacity = 0;
			rhs.m_size = 0;
			rhs.m_tombstones = 0;
		}

		HashMap& operator=(HashMap&& rhs) noexcept
		{
			if (this != &rhs)
			{
				destroy_and_free();
				m_keys = rhs.m_keys;
				m_values = rhs.m_values;
				m_state = rhs.m_state;
				m_capacity = rhs.m_capacity;
				m_size = rhs.m_size;
				m_tombstones = rhs.m_tombstones;
				rhs.m_keys = nullptr;
				rhs.m_values = nullptr;
				rhs.m_state = nullptr;
				rhs.m_capacity = 0;
				rhs.m_size = 0;
				rhs.m_tombstones = 0;
			}
			return *this;
		}

		~HashMap() { destroy_and_free(); }

		// --- Capacity ---

		size_type size()     const noexcept { return m_size; }
		size_type capacity() const noexcept { return m_capacity; }
		bool      empty()    const noexcept { return m_size == 0; }

		void reserve(size_type n)
		{
			size_type needed = round_up_pow2(static_cast<size_type>(n * 4 / 3 + 1));
			if (needed > m_capacity) grow_to(needed);
		}

		// --- Modifiers ---

		void clear()
		{
			if (!m_state) return;
			for (size_type i = 0; i < m_capacity; ++i)
			{
				if (m_state[i] == Occupied)
				{
					if constexpr (!std::is_trivially_destructible_v<Key>)   m_keys[i].~Key();
					if constexpr (!std::is_trivially_destructible_v<T>)     m_values[i].~T();
				}
				m_state[i] = Empty;
			}
			m_size = 0;
			m_tombstones = 0;
		}

		// Insert-or-assign. Returns true if a new entry was created, false if
		// an existing entry was updated.
		bool insert_or_assign(const Key& key, const T& value)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			bool found = locate(key, idx);
			if (found)
			{
				m_values[idx] = value;
				return false;
			}
			// idx is an Empty or Tombstone slot.
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_keys + idx))   Key(key);
			::new (static_cast<void*>(m_values + idx)) T(value);
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		// Insert only if key is absent. Returns true if inserted.
		bool insert(const Key& key, const T& value)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_keys + idx))   Key(key);
			::new (static_cast<void*>(m_values + idx)) T(value);
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		template <class... Args>
		bool emplace(const Key& key, Args&&... args)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_keys + idx))   Key(key);
			::new (static_cast<void*>(m_values + idx)) T(std::forward<Args>(args)...);
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		bool erase(const Key& key)
		{
			if (m_size == 0) return false;
			size_type idx;
			if (!locate(key, idx)) return false;
			if constexpr (!std::is_trivially_destructible_v<Key>) m_keys[idx].~Key();
			if constexpr (!std::is_trivially_destructible_v<T>)   m_values[idx].~T();
			m_state[idx] = Tombstone;
			++m_tombstones;
			--m_size;
			return true;
		}

		void swap(HashMap& other) noexcept
		{
			using std::swap;
			swap(m_keys, other.m_keys);
			swap(m_values, other.m_values);
			swap(m_state, other.m_state);
			swap(m_capacity, other.m_capacity);
			swap(m_size, other.m_size);
			swap(m_tombstones, other.m_tombstones);
		}

		// --- Lookup ---

		T* find(const Key& key)
		{
			if (m_size == 0) return nullptr;
			size_type idx;
			if (locate(key, idx)) return m_values + idx;
			return nullptr;
		}

		const T* find(const Key& key) const
		{
			if (m_size == 0) return nullptr;
			size_type idx;
			if (locate(key, idx)) return m_values + idx;
			return nullptr;
		}

		bool contains(const Key& key) const
		{
			return find(key) != nullptr;
		}

		// Returns reference to existing or default-constructed value.
		T& operator[](const Key& key)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return m_values[idx];
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_keys + idx))   Key(key);
			::new (static_cast<void*>(m_values + idx)) T{};
			m_state[idx] = Occupied;
			++m_size;
			return m_values[idx];
		}

		T& at(const Key& key)
		{
			T* p = find(key);
			assert(p && "HashMap::at: key not found");
			return *p;
		}

		const T& at(const Key& key) const
		{
			const T* p = find(key);
			assert(p && "HashMap::at: key not found");
			return *p;
		}

	private:
		size_type mask() const noexcept { return m_capacity - 1; }

		static size_type round_up_pow2(size_type n)
		{
			if (n <= 1) return 1;
			size_type p = 1;
			while (p < n) p <<= 1;
			return p;
		}

		// Returns true if key found at idx, false if idx is the slot it should go into.
		// Skips tombstones during search but remembers the first one as the insert candidate.
		bool locate(const Key& key, size_type& outIdx) const
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
				if (m_state[i] == Occupied && eq(m_keys[i], key))
				{
					outIdx = i;
					return true;
				}
				if (m_state[i] == Tombstone && firstTombstone == static_cast<size_type>(-1))
					firstTombstone = i;
				i = (i + 1) & m;
			} while (i != start);

			// Fully scanned; no Empty seen — table is degenerate. Should be unreachable
			// if rehash thresholds are respected.
			outIdx = firstTombstone == static_cast<size_type>(-1) ? start : firstTombstone;
			return false;
		}

		// Internal helper used by copy ctor / op= — only inserts into Empty slots,
		// no rehash bookkeeping needed because we've just grown.
		void insert_into_empty_slots(const Key& key, const T& value)
		{
			const size_type m = mask();
			size_type i = Hash{}(key) & m;
			while (m_state[i] != Empty) i = (i + 1) & m;
			::new (static_cast<void*>(m_keys + i))   Key(key);
			::new (static_cast<void*>(m_values + i)) T(value);
			m_state[i] = Occupied;
			++m_size;
		}

		void ensure_capacity_for_one_more()
		{
			// Load factor including tombstones; rehash at 0.75.
			if (m_capacity == 0 || (m_size + m_tombstones + 1) * 4 >= m_capacity * 3)
			{
				grow_to(m_capacity == 0 ? 8 : m_capacity * 2);
			}
		}

		void grow_to(size_type newCap)
		{
			if (newCap <= m_capacity && m_tombstones == 0) return;

			Key*       oldKeys   = m_keys;
			T*         oldValues = m_values;
			uint8_t*   oldState  = m_state;
			size_type  oldCap    = m_capacity;

			m_keys   = m_keyAlloc.allocate(newCap);
			m_values = m_valueAlloc.allocate(newCap);
			m_state  = m_stateAlloc.allocate(newCap);
			for (size_type i = 0; i < newCap; ++i) m_state[i] = Empty;

			const size_type oldSize = m_size;
			m_capacity = newCap;
			m_size = 0;
			m_tombstones = 0;

			if (oldState)
			{
				for (size_type i = 0; i < oldCap; ++i)
				{
					if (oldState[i] == Occupied)
					{
						insert_into_empty_slots(oldKeys[i], oldValues[i]);
						if constexpr (!std::is_trivially_destructible_v<Key>) oldKeys[i].~Key();
						if constexpr (!std::is_trivially_destructible_v<T>)   oldValues[i].~T();
					}
				}
				m_keyAlloc.deallocate(oldKeys, oldCap);
				m_valueAlloc.deallocate(oldValues, oldCap);
				m_stateAlloc.deallocate(oldState, oldCap);
			}
			(void)oldSize;
		}

		void destroy_and_free()
		{
			if (!m_state) return;
			clear();
			m_keyAlloc.deallocate(m_keys, m_capacity);
			m_valueAlloc.deallocate(m_values, m_capacity);
			m_stateAlloc.deallocate(m_state, m_capacity);
			m_keys = nullptr;
			m_values = nullptr;
			m_state = nullptr;
			m_capacity = 0;
		}

		Allocator<Key>     m_keyAlloc{};
		Allocator<T>       m_valueAlloc{};
		Allocator<uint8_t> m_stateAlloc{};
		Key*      m_keys = nullptr;
		T*        m_values = nullptr;
		uint8_t*  m_state = nullptr;
		size_type m_capacity = 0;
		size_type m_size = 0;
		size_type m_tombstones = 0;
	};
}
