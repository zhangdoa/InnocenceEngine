#pragma once
#include "Allocator.h"

#include <cassert>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Inno
{
	// FIFO container backed by a power-of-two circular buffer + Inno::Allocator.
	// Engine-native successor to std::queue for use inside ThreadSafeQueue and
	// direct callers. Front = oldest pushed; back = most-recent pushed.
	// Not thread-safe; caller serialises (ThreadSafeQueue wraps this).
	template <class T>
	class Queue
	{
	public:
		using value_type = T;
		using size_type  = size_t;

		Queue() = default;

		explicit Queue(size_type reserveCount)
		{
			reserve(reserveCount);
		}

		Queue(const Queue& rhs)
		{
			if (rhs.m_size > 0)
			{
				grow_to(round_up_pow2(rhs.m_size));
				for (size_type i = 0; i < rhs.m_size; ++i)
					::new (static_cast<void*>(m_data + i)) T(rhs.peek(i));
				m_size = rhs.m_size;
				m_head = 0;
			}
		}

		Queue& operator=(const Queue& rhs)
		{
			if (this != &rhs)
			{
				clear();
				if (rhs.m_size > 0)
				{
					if (rhs.m_size > m_capacity) grow_to(round_up_pow2(rhs.m_size));
					for (size_type i = 0; i < rhs.m_size; ++i)
						::new (static_cast<void*>(m_data + i)) T(rhs.peek(i));
					m_size = rhs.m_size;
					m_head = 0;
				}
			}
			return *this;
		}

		Queue(Queue&& rhs) noexcept
			: m_data(rhs.m_data)
			, m_capacity(rhs.m_capacity)
			, m_head(rhs.m_head)
			, m_size(rhs.m_size)
		{
			rhs.m_data = nullptr;
			rhs.m_capacity = 0;
			rhs.m_head = 0;
			rhs.m_size = 0;
		}

		Queue& operator=(Queue&& rhs) noexcept
		{
			if (this != &rhs)
			{
				destroy_and_free();
				m_data = rhs.m_data;
				m_capacity = rhs.m_capacity;
				m_head = rhs.m_head;
				m_size = rhs.m_size;
				rhs.m_data = nullptr;
				rhs.m_capacity = 0;
				rhs.m_head = 0;
				rhs.m_size = 0;
			}
			return *this;
		}

		~Queue() { destroy_and_free(); }

		// --- Capacity ---

		size_type size()     const noexcept { return m_size; }
		size_type capacity() const noexcept { return m_capacity; }
		bool      empty()    const noexcept { return m_size == 0; }

		void reserve(size_type n)
		{
			if (n > m_capacity) grow_to(round_up_pow2(n));
		}

		// --- Modifiers ---

		void clear()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (size_type i = 0; i < m_size; ++i) m_data[(m_head + i) & mask()].~T();
			}
			m_head = 0;
			m_size = 0;
		}

		void push(const T& value)
		{
			ensure_one_more();
			::new (static_cast<void*>(slot_at(m_size))) T(value);
			++m_size;
		}

		void push(T&& value)
		{
			ensure_one_more();
			::new (static_cast<void*>(slot_at(m_size))) T(std::move(value));
			++m_size;
		}

		template <class... Args>
		T& emplace(Args&&... args)
		{
			ensure_one_more();
			T* p = ::new (static_cast<void*>(slot_at(m_size))) T(std::forward<Args>(args)...);
			++m_size;
			return *p;
		}

		void pop()
		{
			assert(m_size > 0 && "Queue::pop on empty");
			slot_at(0)->~T();
			m_head = (m_head + 1) & mask();
			--m_size;
		}

		void swap(Queue& other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_capacity, other.m_capacity);
			swap(m_head, other.m_head);
			swap(m_size, other.m_size);
		}

		// --- Element access ---

		T& front()
		{
			assert(m_size > 0 && "Queue::front on empty");
			return *slot_at(0);
		}
		const T& front() const
		{
			assert(m_size > 0 && "Queue::front on empty");
			return *slot_at(0);
		}

		T& back()
		{
			assert(m_size > 0 && "Queue::back on empty");
			return *slot_at(m_size - 1);
		}
		const T& back() const
		{
			assert(m_size > 0 && "Queue::back on empty");
			return *slot_at(m_size - 1);
		}

	private:
		size_type mask() const noexcept { return m_capacity - 1; }

		// Logical index `i` (0 = oldest) → physical slot.
		T* slot_at(size_type i) { return m_data + ((m_head + i) & mask()); }
		const T* slot_at(size_type i) const { return m_data + ((m_head + i) & mask()); }

		const T& peek(size_type i) const { return *slot_at(i); }

		static size_type round_up_pow2(size_type n)
		{
			if (n <= 1) return 1;
			size_type p = 1;
			while (p < n) p <<= 1;
			return p;
		}

		void ensure_one_more()
		{
			if (m_size == m_capacity)
			{
				grow_to(m_capacity == 0 ? 4 : m_capacity * 2);
			}
		}

		void grow_to(size_type newCap)
		{
			if (newCap <= m_capacity) return;
			T* newData = m_alloc.allocate(newCap);
			// Linearise old contents at index 0..m_size in the new buffer.
			if (m_data)
			{
				if constexpr (std::is_trivially_copyable_v<T>)
				{
					// Two memcpys for the wrapped range.
					size_type first = m_capacity - m_head;
					if (first >= m_size)
					{
						std::memcpy(newData, m_data + m_head, m_size * sizeof(T));
					}
					else
					{
						std::memcpy(newData, m_data + m_head, first * sizeof(T));
						std::memcpy(newData + first, m_data, (m_size - first) * sizeof(T));
					}
				}
				else
				{
					for (size_type i = 0; i < m_size; ++i)
					{
						T* src = slot_at(i);
						::new (static_cast<void*>(newData + i)) T(std::move(*src));
						src->~T();
					}
				}
				m_alloc.deallocate(m_data, m_capacity);
			}
			m_data = newData;
			m_capacity = newCap;
			m_head = 0;
		}

		void destroy_and_free()
		{
			if (!m_data) return;
			clear();
			m_alloc.deallocate(m_data, m_capacity);
			m_data = nullptr;
			m_capacity = 0;
		}

		Allocator<T> m_alloc{};
		T*           m_data = nullptr;
		size_type    m_capacity = 0;  // always 0 or a power of 2
		size_type    m_head = 0;
		size_type    m_size = 0;
	};
}
