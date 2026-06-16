// Standalone unit tests for GPUUploadable<T>.
// Decoupled from the engine harness so the contract is testable without
// the engine's Setup+Initialize path.
#include "../../Engine/Common/GPUUploadable.h"
#include "../../Engine/Common/STL17.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <type_traits>
#include <utility>

using namespace Inno;

namespace
{
	struct alignas(16) MirrorPaddedBuffer : public GPUUploadable<MirrorPaddedBuffer>
	{
		float a;
		float b;
		float c;
		float d;
		float e;
		float f;
		float g;
		float h;
		float i;
		float j;
		float k;
		float l;
		float m;
		float n;
		float o;
		float p;
		float q;
		float r;
		float s;
		float t;
		float u;
		float v;
		float w;
		float x;
		uint32_t padding[11];

		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			return { std::pair<size_t, size_t>{ 96, 48 } };
		}
	};

	struct alignas(4) TightBuffer : public GPUUploadable<TightBuffer>
	{
		uint32_t a;
		uint32_t b;
		uint32_t c;
		uint32_t d;
	};
}

namespace gpu_uploadable_test
{
	static int g_Passed = 0;
	static int g_Failed = 0;
	static const char* g_CurrentTest = nullptr;

	static void Record(const char* name, bool ok)
	{
		if (ok)
		{
			++g_Passed;
			std::printf("  [PASS] %s\n", name);
		}
		else
		{
			++g_Failed;
			std::printf("  [FAIL] %s\n", name);
		}
	}
}

#define GPUU_ASSERT(cond) do { \
	if (!(cond)) { \
		std::printf("    assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, false); \
		return; \
	} \
} while (0)

namespace
{
	void TestPoisonInit()
	{
		gpu_uploadable_test::g_CurrentTest = "PoisonInit fills every byte with 0xCD";
		TightBuffer l_buf;
		l_buf.PoisonInit();
		bool l_allPoison = true;
		const uint8_t* l_bytes = reinterpret_cast<const uint8_t*>(&l_buf);
		for (size_t i = 0; i < sizeof(TightBuffer); ++i)
		{
			if (l_bytes[i] != 0xCD) { l_allPoison = false; break; }
		}
		GPUU_ASSERT(l_allPoison);
		GPUU_ASSERT(sizeof(TightBuffer) == 16);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestFirstUnwrittenAllWritten()
	{
		gpu_uploadable_test::g_CurrentTest = "FirstUnwritten returns SIZE_MAX when every dword is written";
		MirrorPaddedBuffer l_buf;
		l_buf.PoisonInit();
		l_buf.a = 1.0f; l_buf.b = 2.0f; l_buf.c = 3.0f; l_buf.d = 4.0f;
		l_buf.e = 5.0f; l_buf.f = 6.0f; l_buf.g = 7.0f; l_buf.h = 8.0f;
		l_buf.i = 9.0f; l_buf.j = 10.0f; l_buf.k = 11.0f; l_buf.l = 12.0f;
		l_buf.m = 13.0f; l_buf.n = 14.0f; l_buf.o = 15.0f; l_buf.p = 16.0f;
		l_buf.q = 17.0f; l_buf.r = 18.0f; l_buf.s = 19.0f; l_buf.t = 20.0f;
		l_buf.u = 21.0f; l_buf.v = 22.0f; l_buf.w = 23.0f; l_buf.x = 24.0f;
		GPUU_ASSERT(l_buf.FirstUnwritten() == SIZE_MAX);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestFirstUnwrittenDroppedField()
	{
		gpu_uploadable_test::g_CurrentTest = "FirstUnwritten returns the byte offset of the first still-poison dword";
		MirrorPaddedBuffer l_buf;
		l_buf.PoisonInit();
		l_buf.a = 1.0f; l_buf.b = 2.0f; l_buf.c = 3.0f; l_buf.d = 4.0f;
		l_buf.e = 5.0f; l_buf.f = 6.0f; l_buf.g = 7.0f; l_buf.h = 8.0f;
		l_buf.i = 9.0f; l_buf.j = 10.0f; l_buf.k = 11.0f; l_buf.l = 12.0f;
		GPUU_ASSERT(l_buf.FirstUnwritten() == 48);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestFirstUnwrittenTightNegative()
	{
		gpu_uploadable_test::g_CurrentTest = "FirstUnwritten on a fully-poisoned tight struct returns 0";
		TightBuffer l_buf;
		l_buf.PoisonInit();
		GPUU_ASSERT(l_buf.FirstUnwritten() == 0);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestFirstUnwrittenTightPartial()
	{
		gpu_uploadable_test::g_CurrentTest = "FirstUnwritten skips written dwords in order";
		TightBuffer l_buf;
		l_buf.PoisonInit();
		l_buf.a = 0x11223344u;
		l_buf.b = 0x55667788u;
		GPUU_ASSERT(l_buf.FirstUnwritten() == 8);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestSkipByteRangesMasksPadding()
	{
		gpu_uploadable_test::g_CurrentTest = "SkipByteRanges masks HLSL std140 padding from the validator";
		MirrorPaddedBuffer l_buf;
		l_buf.PoisonInit();
		l_buf.a = 1.0f; l_buf.b = 2.0f; l_buf.c = 3.0f; l_buf.d = 4.0f;
		l_buf.e = 5.0f; l_buf.f = 6.0f; l_buf.g = 7.0f; l_buf.h = 8.0f;
		l_buf.i = 9.0f; l_buf.j = 10.0f; l_buf.k = 11.0f; l_buf.l = 12.0f;
		l_buf.m = 13.0f; l_buf.n = 14.0f; l_buf.o = 15.0f; l_buf.p = 16.0f;
		l_buf.q = 17.0f; l_buf.r = 18.0f; l_buf.s = 19.0f; l_buf.t = 20.0f;
		l_buf.u = 21.0f; l_buf.v = 22.0f; l_buf.w = 23.0f; l_buf.x = 24.0f;
		GPUU_ASSERT(l_buf.FirstUnwritten() == SIZE_MAX);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestEBOInvariance()
	{
		gpu_uploadable_test::g_CurrentTest = "EBO invariance - sizeof/alignof/standard-layout unchanged vs aggregate without the base";
		struct alignas(16) AggregateNoBase
		{
			float a; float b; float c; float d;
			float e; float f; float g; float h;
			float i; float j; float k; float l;
			float m; float n; float o; float p;
			float q; float r; float s; float t;
			float u; float v; float w; float x;
			uint32_t padding[11];
		};
		GPUU_ASSERT(sizeof(MirrorPaddedBuffer) == sizeof(AggregateNoBase));
		GPUU_ASSERT(alignof(MirrorPaddedBuffer) == alignof(AggregateNoBase));
		GPUU_ASSERT(std::is_standard_layout_v<MirrorPaddedBuffer>);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}

	void TestPoisonDwordConstant()
	{
		gpu_uploadable_test::g_CurrentTest = "PoisonDword is 0xCDCDCDCD";
		GPUU_ASSERT(GPUUploadable<TightBuffer>::PoisonDword == 0xCDCDCDCDu);
		gpu_uploadable_test::Record(gpu_uploadable_test::g_CurrentTest, true);
	}
}

int main()
{
	std::printf("=== GPUUploadable Standalone Unit Tests ===\n");
	TestPoisonInit();
	TestFirstUnwrittenAllWritten();
	TestFirstUnwrittenDroppedField();
	TestFirstUnwrittenTightNegative();
	TestFirstUnwrittenTightPartial();
	TestSkipByteRangesMasksPadding();
	TestEBOInvariance();
	TestPoisonDwordConstant();
	std::printf("=== Results: %d passed, %d failed ===\n",
		gpu_uploadable_test::g_Passed, gpu_uploadable_test::g_Failed);
	return gpu_uploadable_test::g_Failed == 0 ? 0 : 1;
}
