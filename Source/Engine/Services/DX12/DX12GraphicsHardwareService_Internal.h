#pragma once
#include <cstdint>
#include <Windows.h>

namespace Inno
{
	// GPU timer infrastructure constants (TASK-140).
	// Per-queue named-timer ceiling: each named timer claims one stable slot
	// for the lifetime of the run, so this also bounds total distinct pass
	// names ever recorded per queue. 256 leaves headroom for the current
	// ~30-pass renderer plus future RT/GI passes without ever resizing.
	static constexpr uint32_t GPU_TIMER_MAX_NAMED_TIMERS = 256;
	// Number of frames the readback path lags behind the recorded frame.
	// 3 matches the typical swapchain image count and avoids any CPU<->GPU
	// sync stall when reading the most recent fully-completed frame.
	static constexpr uint32_t GPU_TIMER_READBACK_FRAME_LATENCY = 3;
	// Two timestamps per named timer: begin + end.
	static constexpr uint32_t GPU_TIMER_QUERIES_PER_TIMER = 2;
	static constexpr uint32_t GPU_TIMER_TOTAL_QUERIES_PER_QUEUE = GPU_TIMER_MAX_NAMED_TIMERS * GPU_TIMER_QUERIES_PER_TIMER;
	static constexpr UINT64   GPU_TIMER_READBACK_BYTES_PER_QUEUE = GPU_TIMER_TOTAL_QUERIES_PER_QUEUE * sizeof(UINT64);
}
