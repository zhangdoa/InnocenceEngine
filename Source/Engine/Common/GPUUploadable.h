#pragma once
#include "STL17.h"
#include <array>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace Inno
{
	// CRTP mixin for CPU-write / GPU-read structs. Producers call PoisonInit();
	// GPUBufferResourceService::Upload<T> scans for any still-poison dword and
	// logs Error naming the byte offset. HLSL std140 padding cannot be written
	// through C++; derived types override SkipByteRanges() to mask it. EBO,
	// empty base, sizeof(Derived) unchanged vs the non-derived aggregate.
	template <class Derived>
	struct GPUUploadable
	{
		static constexpr uint32_t PoisonDword = 0xCDCDCDCDu;

		void PoisonInit() noexcept
		{
			std::memset(static_cast<Derived*>(this), 0xCD, sizeof(Derived));
		}

		// Overridden by Derived to name HLSL std140 padding regions the
		// producer cannot write (e.g. `uint32_t padding[N]`). Default = none,
		// meaning every byte must be written. Bounded to 4 ranges, header-only.
		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			return {};
		}

		// Returns the byte offset of the first 4-byte word that still matches
		// the poison pattern, or SIZE_MAX if every such word was written.
		size_t FirstUnwritten() const noexcept
		{
			const uint32_t* words = reinterpret_cast<const uint32_t*>(this);
			const size_t wordCount = sizeof(Derived) / sizeof(uint32_t);

			constexpr auto ranges = Derived::SkipByteRanges();
			auto isSkipped = [ranges](size_t byteOffset) noexcept
			{
				for (const auto& range : ranges)
				{
					if (range.first == 0 && range.second == 0)
						break;
					if (byteOffset >= range.first
						&& byteOffset < range.first + range.second)
						return true;
				}
				return false;
			};

			for (size_t i = 0; i < wordCount; ++i)
			{
				const size_t byteOffset = i * sizeof(uint32_t);
				if (isSkipped(byteOffset))
					continue;
				if (words[i] == PoisonDword)
					return byteOffset;
			}
			return SIZE_MAX;
		}
	};
}
