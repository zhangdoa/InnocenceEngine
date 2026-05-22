#pragma once

#include "STL14.h"

namespace Inno
{
	// SPMC double buffer. Contract: ONE producer thread issuing Write + Flip;
	// any number of reader threads issuing Read. Producer must not Write between
	// Flip and the next consumer drain (the post-Flip back is the old front,
	// any in-flight Write to it races with the post-Flip producer write).
	// Sole consumer: WinWindowService::m_WindowEvents.
	template <typename BufferType>
	class DoubleBuffer
	{
	public:
		DoubleBuffer() : m_FrontIndex(0) {}

		DoubleBuffer(const DoubleBuffer&) = delete;
		DoubleBuffer& operator=(const DoubleBuffer&) = delete;

		template <typename Func>
		auto Write(Func&& p_Func)
		{
			std::shared_lock<std::shared_mutex> lock(m_Mutex);
			p_Func(m_Buffers[1 - m_FrontIndex]);
		}

		template <typename Func>
		auto Read(Func&& p_Func) const
		{
			std::shared_lock<std::shared_mutex> lock(m_Mutex);
			p_Func(m_Buffers[m_FrontIndex]);
		}

		void Flip()
		{
			std::unique_lock<std::shared_mutex> lock(m_Mutex);
			m_FrontIndex = 1 - m_FrontIndex;
		}

	private:
		int m_FrontIndex;
		BufferType m_Buffers[2];
		mutable std::shared_mutex m_Mutex;
	};
}
