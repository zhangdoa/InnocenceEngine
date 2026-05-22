#pragma once
#include "STL14.h"
#include "STL17.h"
#include "Template.h"
#include "Memory.h"
#include "Allocator.h"

#include <cassert>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Inno
{
	struct ArrayRangeInfo
	{
		uint64_t m_startOffset;
		uint64_t m_count;
	};

	// Growable, std::vector-like sequence container backed by Inno::Allocator
	// (which routes through Inno::Memory). ThreadSafe=true serialises
	// every public mutator and reader with a shared_mutex; mutators that
	// reallocate (push_back, emplace_back, reserve, resize, shrink_to_fit)
	// hold the unique_lock for the duration of the realloc + move-construct.
	// ThreadSafe=true variants of operator[] / front / back return T by value
	// to avoid returning a reference after the lock destructor releases it.
	template <class T, bool ThreadSafe = false>
	class Array
	{
	public:
		using value_type      = T;
		using size_type       = size_t;
		using reference       = T&;
		using const_reference = const T&;
		using pointer         = T*;
		using const_pointer   = const T*;
		using iterator        = T*;
		using const_iterator  = const T*;

		Array() = default;

		explicit Array(size_type reserveCount)
		{
			reserve(reserveCount);
		}

		Array(const T* first, const T* last)
		{
			const size_type n = static_cast<size_type>(last - first);
			grow_to(n);
			copy_range_init(m_data, first, n);
			m_size = n;
		}

		Array(const Array& rhs)
		{
			grow_to(rhs.m_size);
			copy_range_init(m_data, rhs.m_data, rhs.m_size);
			m_size = rhs.m_size;
		}

		Array& operator=(const Array& rhs)
		{
			if (this != &rhs)
			{
				clear();
				grow_to(rhs.m_size);
				copy_range_init(m_data, rhs.m_data, rhs.m_size);
				m_size = rhs.m_size;
			}
			return *this;
		}

		Array(Array&& rhs) noexcept
			: m_data(rhs.m_data), m_size(rhs.m_size), m_capacity(rhs.m_capacity)
		{
			rhs.m_data = nullptr;
			rhs.m_size = 0;
			rhs.m_capacity = 0;
		}

		Array& operator=(Array&& rhs) noexcept
		{
			if (this != &rhs)
			{
				destroy_and_free();
				m_data = rhs.m_data;
				m_size = rhs.m_size;
				m_capacity = rhs.m_capacity;
				rhs.m_data = nullptr;
				rhs.m_size = 0;
				rhs.m_capacity = 0;
			}
			return *this;
		}

		~Array() { destroy_and_free(); }

		// --- Capacity ---

		size_type size() const noexcept { return m_size; }
		size_type capacity() const noexcept { return m_capacity; }
		bool empty() const noexcept { return m_size == 0; }
		bool is_initialized() const noexcept { return m_data != nullptr; }

		void reserve(size_type newCap)
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				if (newCap > m_capacity) grow_to(newCap);
			}
			else
			{
				if (newCap > m_capacity) grow_to(newCap);
			}
		}

		void resize(size_type newSize)
		{
			resize_impl(newSize, T{});
		}

		void resize(size_type newSize, const T& fill)
		{
			resize_impl(newSize, fill);
		}

		void shrink_to_fit()
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				shrink_impl();
			}
			else
			{
				shrink_impl();
			}
		}

		// Fill currently-uninitialised slots [m_size .. m_capacity) with default-constructed T,
		// then set size = capacity. Backwards-compat helper (RingBuffer uses it as a "treat
		// the whole reserved range as live" hint).
		void fulfill()
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				fulfill_impl();
			}
			else
			{
				fulfill_impl();
			}
		}

		// --- Modifiers ---

		void clear()
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				destroy_range(m_data, m_data + m_size);
				m_size = 0;
			}
			else
			{
				destroy_range(m_data, m_data + m_size);
				m_size = 0;
			}
		}

		void push_back(const T& value)
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				ensure_one_more();
				::new (static_cast<void*>(m_data + m_size)) T(value);
				++m_size;
			}
			else
			{
				ensure_one_more();
				::new (static_cast<void*>(m_data + m_size)) T(value);
				++m_size;
			}
		}

		void push_back(T&& value)
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				ensure_one_more();
				::new (static_cast<void*>(m_data + m_size)) T(std::move(value));
				++m_size;
			}
			else
			{
				ensure_one_more();
				::new (static_cast<void*>(m_data + m_size)) T(std::move(value));
				++m_size;
			}
		}

		template <class... Args>
		T& emplace_back(Args&&... args)
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				ensure_one_more();
				T* p = ::new (static_cast<void*>(m_data + m_size)) T(std::forward<Args>(args)...);
				++m_size;
				return *p;
			}
			else
			{
				ensure_one_more();
				T* p = ::new (static_cast<void*>(m_data + m_size)) T(std::forward<Args>(args)...);
				++m_size;
				return *p;
			}
		}

		void pop_back()
		{
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				assert(m_size > 0 && "pop_back on empty Array");
				--m_size;
				m_data[m_size].~T();
			}
			else
			{
				assert(m_size > 0 && "pop_back on empty Array");
				--m_size;
				m_data[m_size].~T();
			}
		}

		void swap(Array& other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_size, other.m_size);
			swap(m_capacity, other.m_capacity);
		}

		// --- Element access ---

		// ThreadSafe variant returns T by value to avoid returning a reference
		// after the shared_lock's RAII destructor releases it.
		template <typename U = T>
		EnableType<U, ThreadSafe> operator[](size_type pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};
			assert(pos < m_size && "Array::operator[]: out of bounds");
			return m_data[pos];
		}

		template <typename U = T&>
		DisableType<U, ThreadSafe> operator[](size_type pos)
		{
			assert(pos < m_size && "Array::operator[]: out of bounds");
			return m_data[pos];
		}

		template <typename U = T>
		EnableType<U, ThreadSafe> operator[](size_type pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};
			assert(pos < m_size && "Array::operator[] const: out of bounds");
			return m_data[pos];
		}

		template <typename U = const T&>
		DisableType<U, ThreadSafe> operator[](size_type pos) const
		{
			assert(pos < m_size && "Array::operator[] const: out of bounds");
			return m_data[pos];
		}

		T&       at(size_type pos)
		{
			assert(pos < m_size && "Array::at: out of bounds");
			return m_data[pos];
		}
		const T& at(size_type pos) const
		{
			assert(pos < m_size && "Array::at: out of bounds");
			return m_data[pos];
		}

		T&       front()       { assert(m_size > 0 && "Array::front on empty"); return m_data[0]; }
		const T& front() const { assert(m_size > 0 && "Array::front on empty"); return m_data[0]; }
		T&       back()        { assert(m_size > 0 && "Array::back on empty");  return m_data[m_size - 1]; }
		const T& back()  const { assert(m_size > 0 && "Array::back on empty");  return m_data[m_size - 1]; }

		// erase(iterator) — shift-down semantics matching std::vector::erase.
		// Returns iterator one past the erased element (or end() if last erased).
		iterator erase(iterator pos)
		{
			assert(pos >= begin() && pos < end() && "Array::erase: iterator out of bounds");
			const size_type idx = static_cast<size_type>(pos - begin());
			for (size_type i = idx; i + 1 < m_size; ++i)
				m_data[i] = std::move(m_data[i + 1]);
			m_data[m_size - 1].~T();
			--m_size;
			return begin() + idx;
		}

		T*       data() noexcept       { return m_data; }
		const T* data() const noexcept { return m_data; }

		T*       begin() noexcept       { return m_data; }
		T*       end()   noexcept       { return m_data + m_size; }
		const T* begin() const noexcept { return m_data; }
		const T* end()   const noexcept { return m_data + m_size; }

	private:
		void destroy_and_free()
		{
			if (!m_data) return;
			destroy_range(m_data, m_data + m_size);
			m_alloc.deallocate(m_data, m_capacity);
			m_data = nullptr;
			m_size = 0;
			m_capacity = 0;
		}

		void destroy_range(T* first, T* last)
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (T* p = first; p != last; ++p) p->~T();
			}
		}

		void ensure_one_more()
		{
			if (m_size == m_capacity)
			{
				size_type newCap = m_capacity == 0 ? 4 : m_capacity * 2;
				grow_to(newCap);
			}
		}

		// Reallocate to newCap >= m_size; move-construct or memcpy existing elements.
		void grow_to(size_type newCap)
		{
			if (newCap <= m_capacity) return;
			T* newData = m_alloc.allocate(newCap);
			if (m_data)
			{
				if constexpr (std::is_trivially_copyable_v<T>)
				{
					std::memcpy(newData, m_data, m_size * sizeof(T));
				}
				else
				{
					for (size_type i = 0; i < m_size; ++i)
					{
						::new (static_cast<void*>(newData + i)) T(std::move(m_data[i]));
						m_data[i].~T();
					}
				}
				m_alloc.deallocate(m_data, m_capacity);
			}
			m_data = newData;
			m_capacity = newCap;
		}

		void copy_range_init(T* dst, const T* src, size_type n)
		{
			if constexpr (std::is_trivially_copyable_v<T>)
			{
				if (n > 0) std::memcpy(dst, src, n * sizeof(T));
			}
			else
			{
				for (size_type i = 0; i < n; ++i)
					::new (static_cast<void*>(dst + i)) T(src[i]);
			}
		}

		void resize_impl(size_type newSize, const T& fill)
		{
			auto exec = [&]() {
				if (newSize < m_size)
				{
					destroy_range(m_data + newSize, m_data + m_size);
					m_size = newSize;
				}
				else if (newSize > m_size)
				{
					if (newSize > m_capacity) grow_to(newSize);
					for (size_type i = m_size; i < newSize; ++i)
						::new (static_cast<void*>(m_data + i)) T(fill);
					m_size = newSize;
				}
			};
			if constexpr (ThreadSafe)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};
				exec();
			}
			else
			{
				exec();
			}
		}

		void shrink_impl()
		{
			if (m_size == m_capacity) return;
			if (m_size == 0)
			{
				if (m_data) m_alloc.deallocate(m_data, m_capacity);
				m_data = nullptr;
				m_capacity = 0;
				return;
			}
			T* newData = m_alloc.allocate(m_size);
			if constexpr (std::is_trivially_copyable_v<T>)
			{
				std::memcpy(newData, m_data, m_size * sizeof(T));
			}
			else
			{
				for (size_type i = 0; i < m_size; ++i)
				{
					::new (static_cast<void*>(newData + i)) T(std::move(m_data[i]));
					m_data[i].~T();
				}
			}
			m_alloc.deallocate(m_data, m_capacity);
			m_data = newData;
			m_capacity = m_size;
		}

		void fulfill_impl()
		{
			if (m_size < m_capacity)
			{
				for (size_type i = m_size; i < m_capacity; ++i)
					::new (static_cast<void*>(m_data + i)) T();
				m_size = m_capacity;
			}
		}

		Allocator<T> m_alloc{};
		T*           m_data = nullptr;
		size_type    m_size = 0;
		size_type    m_capacity = 0;
		mutable std::shared_mutex m_Mutex;
	};
}
