#pragma once

#include "STL14.h"

namespace Inno
{
	template <typename BufferType>
	class DoubleBuffer
	{
	public:
		DoubleBuffer()
			: m_FrontIndex(0)
			, m_ReadersCount(0)
		{
		}

		DoubleBuffer(const DoubleBuffer&) = delete;
		DoubleBuffer& operator=(const DoubleBuffer&) = delete;

		// Single-producer: no inter-writer locking; back-buffer index is derived from m_FrontIndex.
		template <typename Func>
		auto Write(Func&& p_Func)
		{
			int l_BackIndex = 1 - m_FrontIndex.load(std::memory_order_relaxed);
			p_Func(m_Buffers[l_BackIndex]);
		}

		template <typename Func>
		auto Read(Func&& p_Func) const
		{
			m_ReadersCount.fetch_add(1, std::memory_order_acquire);
			int l_LocalFront = m_FrontIndex.load(std::memory_order_relaxed);
			p_Func(m_Buffers[l_LocalFront]);
			m_ReadersCount.fetch_sub(1, std::memory_order_release);
		}

		void Flip()
		{
			while (m_ReadersCount.load(std::memory_order_acquire) != 0)
				std::this_thread::yield();

			int l_OldFront = m_FrontIndex.load(std::memory_order_relaxed);
			m_FrontIndex.store(1 - l_OldFront, std::memory_order_release);
		}

	private:
		std::atomic<int> m_FrontIndex;
		mutable std::atomic<int> m_ReadersCount;
		BufferType m_Buffers[2];
	};
}
