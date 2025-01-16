#pragma once
#include "STL14.h"
#include "Template.h"
#include "Array.h"

namespace Inno
{
	template <typename T, bool ThreadSafe = false>
	class RingBuffer
	{
	public:
		RingBuffer() = default;
		RingBuffer(size_t elementCount)
		{
			reserve(elementCount);
		}

		~RingBuffer() = default;

		RingBuffer(const RingBuffer<T, ThreadSafe> &rhs)
		{
			m_CurrentElementIndex = rhs.m_CurrentElementIndex;
			m_ElementCount = rhs.m_ElementCount;
			m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
			m_Array = rhs.m_Array;
		}

		RingBuffer<T, ThreadSafe> &operator=(const RingBuffer<T, ThreadSafe> &rhs)
		{
			m_CurrentElementIndex = rhs.m_CurrentElementIndex;
			m_ElementCount = rhs.m_ElementCount;
			m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
			m_Array = rhs.m_Array;
			return *this;
		}

		RingBuffer(RingBuffer<T, ThreadSafe> &&rhs)
		{
			m_CurrentElementIndex = rhs.m_CurrentElementIndex;
			m_ElementCount = rhs.m_ElementCount;
			m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
			m_Array = std::move(rhs.m_Array);
		}

		RingBuffer<T, ThreadSafe> &operator=(RingBuffer<T, ThreadSafe> &&rhs)
		{
			m_CurrentElementIndex = rhs.m_CurrentElementIndex;
			m_ElementCount = rhs.m_ElementCount;
			m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
			m_Array = std::move(rhs.m_Array);
			return *this;
		}

		template <typename U = T &>
		EnableType<U, ThreadSafe> operator[](size_t pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			return m_Array[pos % m_ElementCount];
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> operator[](size_t pos)
		{
			return m_Array[pos % m_ElementCount];
		}

		template <typename U = const T &>
		EnableType<U, ThreadSafe> operator[](size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			return m_Array[pos % m_ElementCount];
		}

		template <typename U = const T &>
		DisableType<U, ThreadSafe> operator[](size_t pos) const
		{
			return m_Array[pos % m_ElementCount];
		}

		auto reserve(size_t elementCount)
		{
			m_ElementCount = elementCount;
			m_Array.reserve(m_ElementCount);
			m_Array.fulfill();
		}

		const auto capacity() const
		{
			return m_ElementCount;
		}

		const auto size() const
		{
			if (m_isLoopingOverOnce)
			{
				return m_ElementCount;
			}
			else
			{
				return m_CurrentElementIndex;
			}
		}

		const auto currentElementPos() const
		{
			return m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1;
		}

		template <typename U = T &>
		EnableType<U, ThreadSafe> currentElement()
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> currentElement()
		{
			return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
		}

		template <typename U = const T &>
		EnableType<U, ThreadSafe> currentElement() const
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
		}

		template <typename U = const T &>
		DisableType<U, ThreadSafe> currentElement() const
		{
			return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
		}

		template <typename U = void>
		EnableType<U, ThreadSafe> emplace_back(const T &value)
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			m_Array[m_CurrentElementIndex] = value;

			m_CurrentElementIndex++;

			if (m_CurrentElementIndex >= m_ElementCount)
			{
				m_CurrentElementIndex = 0;
				m_isLoopingOverOnce = true;
			}
		}

		template <typename U = void>
		DisableType<U, ThreadSafe> emplace_back(const T &value)
		{
			m_Array[m_CurrentElementIndex] = value;

			m_CurrentElementIndex++;

			if (m_CurrentElementIndex >= m_ElementCount)
			{
				m_CurrentElementIndex = 0;
				m_isLoopingOverOnce = true;
			}
		}

	private:
		size_t m_CurrentElementIndex = 0;
		size_t m_ElementCount = 0;
		bool m_isLoopingOverOnce = false;
		Array<T, ThreadSafe> m_Array;
		mutable std::shared_mutex m_Mutex;
	};
}