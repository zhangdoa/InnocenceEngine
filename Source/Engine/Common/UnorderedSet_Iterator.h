#pragma once
#include <cstddef>
#include <iterator>

namespace Inno
{
	namespace UnorderedSetDetail
	{
		// Matches UnorderedSetStorage::SlotState::Occupied; kept in sync via static_assert.
		constexpr uint8_t kOccupied = 1;
	}

	template <class Set>
	class UnorderedSetIterator
	{
	public:
		using value_type        = typename Set::value_type;
		using reference         = const value_type&;
		using pointer           = const value_type*;
		using difference_type   = std::ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		using size_type         = size_t;

		UnorderedSetIterator() = default;
		UnorderedSetIterator(const Set* s, size_type idx) : m_set(s), m_idx(idx) {}

		reference operator*()  const { return m_set->m_data[m_idx]; }
		pointer   operator->() const { return m_set->m_data + m_idx; }

		UnorderedSetIterator& operator++()
		{
			++m_idx;
			while (m_idx < m_set->m_capacity && m_set->m_state[m_idx] != UnorderedSetDetail::kOccupied)
				++m_idx;
			return *this;
		}

		UnorderedSetIterator operator++(int) { UnorderedSetIterator tmp = *this; ++*this; return tmp; }

		bool operator==(const UnorderedSetIterator& other) const { return m_idx == other.m_idx && m_set == other.m_set; }
		bool operator!=(const UnorderedSetIterator& other) const { return !(*this == other); }

		size_type index() const { return m_idx; }
		const Set* container() const { return m_set; }

	private:
		const Set* m_set = nullptr;
		size_type  m_idx = 0;
	};
}
