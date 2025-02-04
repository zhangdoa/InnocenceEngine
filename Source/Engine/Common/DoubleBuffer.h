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

		// Non-copyable and non-movable to avoid accidental duplication
		DoubleBuffer(const DoubleBuffer&) = delete;
		DoubleBuffer& operator=(const DoubleBuffer&) = delete;

		/**
		 * Write to the back buffer.
		 * The producer calls this to modify the buffer that is NOT currently visible to readers.
		 */
		template <typename Func>
		auto Write(Func&& p_Func)
		{
			// Single-producer assumption: no need for locks among multiple writers
			int l_BackIndex = 1 - m_FrontIndex.load(std::memory_order_relaxed);
			p_Func(m_Buffers[l_BackIndex]);
		}

		/**
		 * Read from the front buffer.
		 * Each consumer calls this to safely inspect the buffer that is currently 'front.'
		 */
		template <typename Func>
		auto Read(Func&& p_Func) const
		{
			// Indicate a read is active
			m_ReadersCount.fetch_add(1, std::memory_order_acquire);

			// Snapshot the front index once
			int l_LocalFront = m_FrontIndex.load(std::memory_order_relaxed);

			// Perform the read
			p_Func(m_Buffers[l_LocalFront]);

			// Indicate this read is done
			m_ReadersCount.fetch_sub(1, std::memory_order_release);
		}

		/**
		 * Flip the front and back buffers.
		 * The producer calls this once (or more) per frame/interval.
		 */
		void Flip()
		{
			// Wait for all readers to finish
			while (m_ReadersCount.load(std::memory_order_acquire) != 0)
			{
				// Busy-wait spin
				std::this_thread::yield();
			}

			// Flip front/back
			int l_OldFront = m_FrontIndex.load(std::memory_order_relaxed);
			m_FrontIndex.store(1 - l_OldFront, std::memory_order_release);
		}

	private:
		// 0 or 1 - index of current front buffer
		std::atomic<int> m_FrontIndex;

		// Number of active readers
		mutable std::atomic<int> m_ReadersCount;

		// Storage for two buffers
		BufferType m_Buffers[2];
	};
}
