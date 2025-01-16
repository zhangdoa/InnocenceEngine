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
		Array() = default;
		Array(size_t elementCount)
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
			}
		}

		Array(const Array<T, ThreadSafe> &rhs)
		{
			m_ElementSize = rhs.m_ElementSize;
			m_ElementCount = rhs.m_ElementCount;
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
		}

		Array<T, ThreadSafe> &operator=(const Array<T, ThreadSafe> &rhs)
		{
			m_ElementSize = rhs.m_ElementSize;
			m_ElementCount = rhs.m_ElementCount;
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
			return *this;
		}

		Array(Array<T, ThreadSafe> &&rhs)
		{
			m_ElementSize = rhs.m_ElementSize;
			m_ElementCount = rhs.m_ElementCount;
			m_HeapAddress = rhs.m_HeapAddress;
			rhs.m_HeapAddress = nullptr;
			m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
		}

		Array<T, ThreadSafe> &operator=(Array<T, ThreadSafe> &&rhs)
		{
			m_ElementSize = rhs.m_ElementSize;
			m_ElementCount = rhs.m_ElementCount;
			m_HeapAddress = rhs.m_HeapAddress;
			rhs.m_HeapAddress = nullptr;
			m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
			return *this;
		}

		Array(const T *begin, const T *end)
		{
			m_ElementSize = sizeof(T);
			m_ElementCount = (end - begin);
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memcpy(m_HeapAddress, begin, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = m_ElementCount;
		}

		template <typename U = T &>
		EnableType<U, ThreadSafe> operator[](size_t pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> operator[](size_t pos)
		{
			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = const T &>
		EnableType<U, ThreadSafe> operator[](size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		template <typename U = const T &>
		DisableType<U, ThreadSafe> operator[](size_t pos) const
		{
			assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
			assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

			return *(m_HeapAddress + pos);
		}

		auto begin()
		{
			return m_HeapAddress;
		}

		auto end()
		{
			return m_HeapAddress + m_ElementCount;
		}

		const auto begin() const
		{
			return m_HeapAddress;
		}

		const auto end() const
		{
			return m_HeapAddress + m_ElementCount;
		}

		const auto capacity() const
		{
			return m_ElementCount;
		}

		const auto size() const
		{
			return m_CurrentFreeIndex;
		}

		auto reserve(size_t elementCount)
		{
			m_ElementSize = sizeof(T);
			m_ElementCount = elementCount;
			m_HeapAddress = reinterpret_cast<T *>(Memory::Allocate(m_ElementCount * m_ElementSize));
			std::memset(m_HeapAddress, 0, m_ElementCount * m_ElementSize);
			m_CurrentFreeIndex = 0;
		}

		auto fulfill()
		{
			std::fill(m_HeapAddress, m_HeapAddress + m_ElementCount, T());
			m_CurrentFreeIndex = m_ElementCount;
		}

		auto clear()
		{
			m_CurrentFreeIndex = 0;
		}

		template <typename U = void>
		EnableType<U, ThreadSafe> emplace_back(const T &value)
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			assert(m_CurrentFreeIndex <= m_ElementCount && "Heap overflow occurred due to out-of-boundary emplace back operation.");

			*(m_HeapAddress + m_CurrentFreeIndex) = value;
			m_CurrentFreeIndex++;
		}

		template <typename U = void>
		DisableType<U, ThreadSafe> emplace_back(const T &value)
		{
			assert(m_CurrentFreeIndex <= m_ElementCount && "Heap overflow occurred due to out-of-boundary emplace back operation.");

			*(m_HeapAddress + m_CurrentFreeIndex) = value;
			m_CurrentFreeIndex++;
		}

	private:
		T *m_HeapAddress;
		size_t m_ElementSize;
		size_t m_ElementCount;
		size_t m_CurrentFreeIndex;
		std::shared_mutex m_Mutex;
	};
}