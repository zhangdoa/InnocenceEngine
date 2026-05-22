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
	// Storage is std::pair<Key, T> so iterator deref yields a real pair&
	// matching the std::unordered_map shape. Caller must not mutate .first
	// via the iterator (UB — same contract as std::unordered_map).
	// Not thread-safe; caller serialises (ThreadSafeUnorderedMap wraps this).
	template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
	class HashMap
	{
		enum SlotState : uint8_t { Empty = 0, Occupied = 1, Tombstone = 2 };

	public:
		using key_type    = Key;
		using mapped_type = T;
		using value_type  = std::pair<Key, T>;
		using size_type   = size_t;

	private:
		template <bool IsConst>
		class iter_t
		{
			using Map = std::conditional_t<IsConst, const HashMap, HashMap>;
		public:
			using value_type        = HashMap::value_type;
			using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;
			using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
			using difference_type   = std::ptrdiff_t;
			using iterator_category = std::forward_iterator_tag;

			iter_t() = default;
			iter_t(Map* m, size_type idx) : m_map(m), m_idx(idx) {}

			reference operator*()  const { return m_map->m_data[m_idx]; }
			pointer   operator->() const { return m_map->m_data + m_idx; }

			iter_t& operator++()
			{
				++m_idx;
				while (m_idx < m_map->m_capacity && m_map->m_state[m_idx] != Occupied)
					++m_idx;
				return *this;
			}

			iter_t operator++(int) { iter_t tmp = *this; ++*this; return tmp; }

			bool operator==(const iter_t& other) const { return m_idx == other.m_idx && m_map == other.m_map; }
			bool operator!=(const iter_t& other) const { return !(*this == other); }

			// Allow comparison of iterator with const_iterator.
			template <bool OtherConst>
			bool operator==(const iter_t<OtherConst>& other) const { return m_idx == other.m_idx && m_map == other.m_map; }
			template <bool OtherConst>
			bool operator!=(const iter_t<OtherConst>& other) const { return !(*this == other); }

			template <bool> friend class iter_t;
			friend class HashMap;

		private:
			Map*      m_map = nullptr;
			size_type m_idx = 0;
		};

	public:
		using iterator       = iter_t<false>;
		using const_iterator = iter_t<true>;

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
					insert_into_empty_slots(rhs.m_data[i].first, rhs.m_data[i].second);
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
						insert_into_empty_slots(rhs.m_data[i].first, rhs.m_data[i].second);
				}
			}
			return *this;
		}

		HashMap(HashMap&& rhs) noexcept
			: m_data(rhs.m_data), m_state(rhs.m_state)
			, m_capacity(rhs.m_capacity), m_size(rhs.m_size), m_tombstones(rhs.m_tombstones)
		{
			rhs.m_data = nullptr;
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
				m_data = rhs.m_data;
				m_state = rhs.m_state;
				m_capacity = rhs.m_capacity;
				m_size = rhs.m_size;
				m_tombstones = rhs.m_tombstones;
				rhs.m_data = nullptr;
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

		// --- Iterators ---

		iterator       begin()        { return iterator(this, first_occupied(0)); }
		const_iterator begin()  const { return const_iterator(this, first_occupied(0)); }
		const_iterator cbegin() const { return begin(); }
		iterator       end()          { return iterator(this, m_capacity); }
		const_iterator end()    const { return const_iterator(this, m_capacity); }
		const_iterator cend()   const { return end(); }

		// --- Modifiers ---

		void clear()
		{
			if (!m_state) return;
			for (size_type i = 0; i < m_capacity; ++i)
			{
				if (m_state[i] == Occupied)
				{
					if constexpr (!std::is_trivially_destructible_v<value_type>) m_data[i].~value_type();
				}
				m_state[i] = Empty;
			}
			m_size = 0;
			m_tombstones = 0;
		}

		// Insert-or-assign. Returns true if a new entry was created.
		bool insert_or_assign(const Key& key, const T& value)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			bool found = locate(key, idx);
			if (found)
			{
				m_data[idx].second = value;
				return false;
			}
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(key, value);
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		bool insert(const Key& key, const T& value)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(key, value);
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
			::new (static_cast<void*>(m_data + idx)) value_type(
				std::piecewise_construct,
				std::forward_as_tuple(key),
				std::forward_as_tuple(std::forward<Args>(args)...));
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		bool erase(const Key& key)
		{
			if (m_size == 0) return false;
			size_type idx;
			if (!locate(key, idx)) return false;
			if constexpr (!std::is_trivially_destructible_v<value_type>) m_data[idx].~value_type();
			m_state[idx] = Tombstone;
			++m_tombstones;
			--m_size;
			return true;
		}

		// Erase by iterator. Returns next iterator (or end()).
		iterator erase(iterator it)
		{
			assert(it.m_map == this && it.m_idx < m_capacity && m_state[it.m_idx] == Occupied);
			if constexpr (!std::is_trivially_destructible_v<value_type>) m_data[it.m_idx].~value_type();
			m_state[it.m_idx] = Tombstone;
			++m_tombstones;
			--m_size;
			return iterator(this, first_occupied(it.m_idx + 1));
		}

		void swap(HashMap& other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_state, other.m_state);
			swap(m_capacity, other.m_capacity);
			swap(m_size, other.m_size);
			swap(m_tombstones, other.m_tombstones);
		}

		// --- Lookup ---

		iterator find(const Key& key)
		{
			if (m_size == 0) return end();
			size_type idx;
			if (locate(key, idx)) return iterator(this, idx);
			return end();
		}

		const_iterator find(const Key& key) const
		{
			if (m_size == 0) return end();
			size_type idx;
			if (locate(key, idx)) return const_iterator(this, idx);
			return end();
		}

		bool contains(const Key& key) const
		{
			if (m_size == 0) return false;
			size_type idx;
			return locate(key, idx);
		}

		// operator[] inserts default on miss.
		T& operator[](const Key& key)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return m_data[idx].second;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(key, T{});
			m_state[idx] = Occupied;
			++m_size;
			return m_data[idx].second;
		}

		T& at(const Key& key)
		{
			auto it = find(key);
			assert(it != end() && "HashMap::at: key not found");
			return it->second;
		}

		const T& at(const Key& key) const
		{
			auto it = find(key);
			assert(it != end() && "HashMap::at: key not found");
			return it->second;
		}

	private:
		template <bool> friend class iter_t;

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
				if (m_state[i] == Occupied && eq(m_data[i].first, key))
				{
					outIdx = i;
					return true;
				}
				if (m_state[i] == Tombstone && firstTombstone == static_cast<size_type>(-1))
					firstTombstone = i;
				i = (i + 1) & m;
			} while (i != start);

			outIdx = firstTombstone == static_cast<size_type>(-1) ? start : firstTombstone;
			return false;
		}

		// Universal-references + forwarding so move-only T (e.g. std::unique_ptr)
		// works on grow_to's move path; lvalue callers (copy ctor) still copy.
		template <class K, class V>
		void insert_into_empty_slots(K&& key, V&& value)
		{
			const size_type m = mask();
			size_type i = Hash{}(key) & m;
			while (m_state[i] != Empty) i = (i + 1) & m;
			::new (static_cast<void*>(m_data + i)) value_type(std::forward<K>(key), std::forward<V>(value));
			m_state[i] = Occupied;
			++m_size;
		}

		void ensure_capacity_for_one_more()
		{
			if (m_capacity == 0 || (m_size + m_tombstones + 1) * 4 >= m_capacity * 3)
			{
				grow_to(m_capacity == 0 ? 8 : m_capacity * 2);
			}
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
						insert_into_empty_slots(std::move(oldData[i].first), std::move(oldData[i].second));
						if constexpr (!std::is_trivially_destructible_v<value_type>) oldData[i].~value_type();
					}
				}
				m_dataAlloc.deallocate(oldData, oldCap);
				m_stateAlloc.deallocate(oldState, oldCap);
			}
		}

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

		Allocator<value_type> m_dataAlloc{};
		Allocator<uint8_t>    m_stateAlloc{};
		value_type* m_data = nullptr;
		uint8_t*    m_state = nullptr;
		size_type   m_capacity = 0;
		size_type   m_size = 0;
		size_type   m_tombstones = 0;
	};
}
