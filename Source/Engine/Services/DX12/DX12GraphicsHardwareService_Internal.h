#pragma once
#include <cstdint>
#include <Windows.h>

namespace Inno
{
	// Per-queue ceiling on distinct pass names ever recorded; each named timer
	// claims one stable slot for the lifetime of the run.
	static constexpr uint32_t GPU_TIMER_MAX_NAMED_TIMERS = 256;
	// Matches the typical swapchain image count so reads of the most recent
	// completed frame never stall.
	static constexpr uint32_t GPU_TIMER_READBACK_FRAME_LATENCY = 3;
	// Two timestamps per named timer: begin + end.
	static constexpr uint32_t GPU_TIMER_QUERIES_PER_TIMER = 2;
	static constexpr uint32_t GPU_TIMER_TOTAL_QUERIES_PER_QUEUE = GPU_TIMER_MAX_NAMED_TIMERS * GPU_TIMER_QUERIES_PER_TIMER;
	static constexpr UINT64   GPU_TIMER_READBACK_BYTES_PER_QUEUE = GPU_TIMER_TOTAL_QUERIES_PER_QUEUE * sizeof(UINT64);
}
