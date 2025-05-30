#pragma once
#include "STL14.h"
#include "STL17.h"
#include "Template.h"
#include "Memory.h"

namespace Inno
{
    struct ArrayRangeInfo
	{
		uint64_t m_startOffset;
		uint64_t m_count;
	};

	template <class T, bool ThreadSafe = false>
	class Array
	{
	public:
		Array() : m_HeapAddress(nullptr), m_ElementSize(0), m_ElementCount(0), m_CurrentFreeIndex(0), m_Initialized(false)
		{
		}
		Array(size_t elementCount) : Array()
		{
			reserve(elementCount);
		}

		~Array()
		{
			if (m_HeapAddress)
			{
				Memory::Deallocate(m_HeapAddress);
				m_HeapAddress = nullptr;
				m_CurrentFreeIndex = 0;
				m_ElementCount = 0;
				m_ElementSize = 0;
				m_Initialized = false;
			}
		}

		Array(const Array<T, ThreadSafe> &rhs) : Array()
		{
			if (rhs.m_Initialized)
			{
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
				std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
				m_Initialized = true;
			}
		}

		Array<T, ThreadSafe> &operator=(const Array<T, ThreadSafe> &rhs)
		{
			if (this != &rhs)
			{
				// Deallocate existing memory to prevent leak
				if (m_HeapAddress)
				{
					Memory::Deallocate(m_HeapAddress);
					m_HeapAddress = nullptr;
					m_Initialized = false;
				}
				
				if (rhs.m_Initialized)
				{
					m_ElementSize = rhs.m_ElementSize;
					m_ElementCount = rhs.m_ElementCount;
					m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
					std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
					m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
					m_Initialized = true;
				}
			}
			return *this;
		}

		Array(Array<T, ThreadSafe> &&rhs) : Array()
		{
			m_ElementSize = rhs.m_ElementSize;
			m_ElementCount = rhs.m_ElementCount;
			m_HeapAddress = rhs.m_HeapAddress;
			m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
			m_Initialized = rhs.m_Initialized;
			
			rhs.m_HeapAddress = nullptr;
			rhs.m_Initialized = false;
		}

		Array<T, ThreadSafe> &operator=(Array<T, ThreadSafe> &&rhs)
		{
			if (this != &rhs)
			{
				// Clean up existing resources
				if (m_HeapAddress)
				{
					Memory::Deallocate(m_HeapAddress);
				}
				
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = rhs.m_HeapAddress;
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
				m_Initialized = rhs.m_Initialized;
				
				rhs.m_HeapAddress = nullptr;
				rhs.m_Initialized = false;
			}
			return *this;
		}

		Array(const T *begin, const T *end) : Array()
		{
			m_ElementSize = sizeof(T);
			m_ElementCount = (end - begin);
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memcpy(m_HeapAddress, begin, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = m_ElementCount;
			m_Initialized = true;
		}

		template <typename U = T &>
		EnableType<U, ThreadSafe> operator[](size_t pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};
			ensureUsable();

			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos < m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> operator[](size_t pos)
		{
			ensureUsable();
			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos < m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = const T &>
		EnableType<U, ThreadSafe> operator[](size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};
			ensureUsable();

			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos < m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = const T &>
		DisableType<U, ThreadSafe> operator[](size_t pos) const
		{
			ensureUsable();
			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos < m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		auto begin()
		{
			return m_Initialized ? m_HeapAddress : static_cast<T*>(nullptr);
		}

		auto end()
		{
			return m_Initialized ? (m_HeapAddress + m_CurrentFreeIndex) : static_cast<T*>(nullptr);
		}

		const auto begin() const
		{
			return m_Initialized ? m_HeapAddress : static_cast<const T*>(nullptr);
		}

		const auto end() const
		{
			return m_Initialized ? (m_HeapAddress + m_CurrentFreeIndex) : static_cast<const T*>(nullptr);
		}

		const auto capacity() const
		{
			return m_Initialized ? m_ElementCount : 0;
		}

		const auto size() const
		{
			return m_Initialized ? m_CurrentFreeIndex : 0;
		}

		const auto is_initialized() const
		{
			return m_Initialized;
		}

		auto reserve(size_t elementCount)
		{
			assert(!m_Initialized && "Array capacity already fixed - cannot reserve again");
			assert(elementCount > 0 && "Cannot reserve zero elements");
			
			m_ElementSize = sizeof(T);
			m_ElementCount = elementCount;
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memset(m_HeapAddress, 0, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = 0;
			m_Initialized = true;
		}

		auto fulfill()
		{
			ensureUsable();
			std::fill(m_HeapAddress, m_HeapAddress + m_ElementCount, T());
			m_CurrentFreeIndex = m_ElementCount;
		}

		auto clear()
		{
			if (m_Initialized)
			{
				m_CurrentFreeIndex = 0;
			}
		}

		template <typename U = void>
		EnableType<U, ThreadSafe> emplace_back(const T &value)
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};
			ensureUsable();

			assert(m_CurrentFreeIndex < m_ElementCount && "Fixed capacity exceeded - cannot emplace beyond reserved size.");

			*(m_HeapAddress + m_CurrentFreeIndex) = value;
			m_CurrentFreeIndex++;
		}

		template <typename U = void>
		DisableType<U, ThreadSafe> emplace_back(const T &value)
		{
			ensureUsable();
			assert(m_CurrentFreeIndex < m_ElementCount && "Fixed capacity exceeded - cannot emplace beyond reserved size.");

			*(m_HeapAddress + m_CurrentFreeIndex) = value;
			m_CurrentFreeIndex++;
		}

	private:
		void ensureUsable() const
		{
			assert(m_Initialized && "Array not initialized - call reserve() first");
		}
		
		T *m_HeapAddress;
		size_t m_ElementSize;
		size_t m_ElementCount;
		size_t m_CurrentFreeIndex;
		bool m_Initialized;
		std::shared_mutex m_Mutex;
	};
}