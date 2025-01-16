#pragma once
#include "STL14.h"
#include "Template.h"

namespace Inno
{
	template <typename T, bool ThreadSafe = false>
	class DoubleBuffer
	{
	public:
		DoubleBuffer() = default;
		~DoubleBuffer() = default;

		template <typename U = T &>
		EnableType<U, ThreadSafe> GetOldValue()
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			if (m_IsAReady)
			{
				return m_B;
			}
			else
			{
				return m_A;
			}
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> GetOldValue()
		{
			if (m_IsAReady)
			{
				return m_B;
			}
			else
			{
				return m_A;
			}
		}

		template <typename U = T &>
		EnableType<U, ThreadSafe> GetNewValue()
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			if (m_IsAReady)
			{
				return m_A;
			}
			else
			{
				return m_B;
			}
		}

		template <typename U = T &>
		DisableType<U, ThreadSafe> GetNewValue()
		{
			if (m_IsAReady)
			{
				return m_A;
			}
			else
			{
				return m_B;
			}
		}

		template <typename U = void>
		EnableType<U, ThreadSafe> SetValue(T &&value)
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			if (m_IsAReady)
			{
				m_B = std::move(value);
				m_IsAReady = false;
			}
			else
			{
				m_A = std::move(value);
				m_IsAReady = true;
			}
		}

		template <typename U = void>
		DisableType<U, ThreadSafe> SetValue(T &&value)
		{
			if (m_IsAReady)
			{
				m_B = std::move(value);
				m_IsAReady = false;
			}
			else
			{
				m_A = std::move(value);
				m_IsAReady = true;
			}
		}

		template <typename U = void>
		EnableType<U, ThreadSafe> Reserve(size_t elementCount)
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			m_A.reserve(elementCount);
			m_B.reserve(elementCount);
		}

		template <typename U = void>
		DisableType<U, ThreadSafe> Reserve(size_t elementCount)
		{
			m_A.reserve(elementCount);
			m_B.reserve(elementCount);
		}

	private:
		std::atomic<bool> m_IsAReady = false;
		mutable std::shared_mutex m_Mutex;
		T m_A;
		T m_B;
	};
}