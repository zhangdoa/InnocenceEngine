#pragma once
#include "UnorderedSet_Storage.h"
#include "UnorderedSet_Iterator.h"

#include <functional>
#include <initializer_list>
#include <utility>

namespace Inno
{
	// Open-addressing set peer of Inno::HashMap. Iterator yields const T& —
	// mutating an element through it would break the hash invariant (UB).
	template <class T, class Hash = std::hash<T>, class KeyEqual = std::equal_to<T>>
	class UnorderedSet : private UnorderedSetStorage<T, Hash, KeyEqual>
	{
		using Base = UnorderedSetStorage<T, Hash, KeyEqual>;
		using Base::Empty; using Base::Occupied; using Base::Tombstone;
		using Base::m_data; using Base::m_state; using Base::m_capacity;
		using Base::m_size; using Base::m_tombstones;
		using Base::round_up_pow2; using Base::first_occupied;
		using Base::locate; using Base::insert_into_empty_slots;
		using Base::ensure_capacity_for_one_more; using Base::grow_to; using Base::destroy_and_free;

		static_assert(Base::Occupied == UnorderedSetDetail::kOccupied, "kOccupied / SlotState::Occupied drift");

	public:
		using Base::clear;
		using key_type    = T;
		using value_type  = T;
		using size_type   = size_t;
		using iterator       = UnorderedSetIterator<UnorderedSet>;
		using const_iterator = UnorderedSetIterator<UnorderedSet>;

		friend class UnorderedSetIterator<UnorderedSet>;

		UnorderedSet() = default;
		explicit UnorderedSet(size_type initialCapacity) { reserve(initialCapacity); }

		UnorderedSet(std::initializer_list<T> il)
		{
			reserve(il.size());
			for (const auto& v : il) insert(v);
		}

		template <class InputIt>
		UnorderedSet(InputIt first, InputIt last)
		{
			for (; first != last; ++first) insert(*first);
		}

		UnorderedSet(const UnorderedSet& rhs)
		{
			if (rhs.m_size == 0) return;
			grow_to(rhs.m_capacity);
			for (size_type i = 0; i < rhs.m_capacity; ++i)
				if (rhs.m_state[i] == Occupied) insert_into_empty_slots(rhs.m_data[i]);
		}

		UnorderedSet& operator=(const UnorderedSet& rhs)
		{
			if (this != &rhs)
			{
				clear();
				if (rhs.m_size == 0) return *this;
				if (m_capacity < rhs.m_capacity) grow_to(rhs.m_capacity);
				for (size_type i = 0; i < rhs.m_capacity; ++i)
					if (rhs.m_state[i] == Occupied) insert_into_empty_slots(rhs.m_data[i]);
			}
			return *this;
		}

		UnorderedSet(UnorderedSet&& rhs) noexcept
		{
			m_data = rhs.m_data; m_state = rhs.m_state;
			m_capacity = rhs.m_capacity; m_size = rhs.m_size; m_tombstones = rhs.m_tombstones;
			rhs.m_data = nullptr; rhs.m_state = nullptr;
			rhs.m_capacity = 0; rhs.m_size = 0; rhs.m_tombstones = 0;
		}

		UnorderedSet& operator=(UnorderedSet&& rhs) noexcept
		{
			if (this != &rhs)
			{
				destroy_and_free();
				m_data = rhs.m_data; m_state = rhs.m_state;
				m_capacity = rhs.m_capacity; m_size = rhs.m_size; m_tombstones = rhs.m_tombstones;
				rhs.m_data = nullptr; rhs.m_state = nullptr;
				rhs.m_capacity = 0; rhs.m_size = 0; rhs.m_tombstones = 0;
			}
			return *this;
		}

		size_type size()     const noexcept { return m_size; }
		size_type capacity() const noexcept { return m_capacity; }
		bool      empty()    const noexcept { return m_size == 0; }

		void reserve(size_type n)
		{
			size_type needed = round_up_pow2(static_cast<size_type>(n * 4 / 3 + 1));
			if (needed > m_capacity) grow_to(needed);
		}

		iterator       begin()        { return iterator(this, first_occupied(0)); }
		const_iterator begin()  const { return const_iterator(this, first_occupied(0)); }
		const_iterator cbegin() const { return begin(); }
		iterator       end()          { return iterator(this, m_capacity); }
		const_iterator end()    const { return const_iterator(this, m_capacity); }
		const_iterator cend()   const { return end(); }

		bool insert(const T& key)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(key);
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		bool insert(T&& key)
		{
			ensure_capacity_for_one_more();
			size_type idx;
			if (locate(key, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(std::move(key));
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		template <class... Args>
		bool emplace(Args&&... args)
		{
			ensure_capacity_for_one_more();
			value_type tmp(std::forward<Args>(args)...);
			size_type idx;
			if (locate(tmp, idx)) return false;
			if (m_state[idx] == Tombstone) --m_tombstones;
			::new (static_cast<void*>(m_data + idx)) value_type(std::move(tmp));
			m_state[idx] = Occupied;
			++m_size;
			return true;
		}

		bool erase(const T& key)
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

		iterator erase(iterator it)
		{
			const size_type idx = it.index();
			assert(it.container() == this && idx < m_capacity && m_state[idx] == Occupied);
			if constexpr (!std::is_trivially_destructible_v<value_type>) m_data[idx].~value_type();
			m_state[idx] = Tombstone;
			++m_tombstones;
			--m_size;
			return iterator(this, first_occupied(idx + 1));
		}

		void swap(UnorderedSet& other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data); swap(m_state, other.m_state);
			swap(m_capacity, other.m_capacity); swap(m_size, other.m_size); swap(m_tombstones, other.m_tombstones);
		}

		iterator find(const T& key)
		{
			if (m_size == 0) return end();
			size_type idx;
			if (locate(key, idx)) return iterator(this, idx);
			return end();
		}

		const_iterator find(const T& key) const
		{
			if (m_size == 0) return end();
			size_type idx;
			if (locate(key, idx)) return const_iterator(this, idx);
			return end();
		}

		bool contains(const T& key) const
		{
			if (m_size == 0) return false;
			size_type idx;
			return locate(key, idx);
		}
	};
}
