// https://github.com/miloyip/itoa-benchmark
const char gDigitsLut[200] = {
	'0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
	'1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
	'2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
	'3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
	'4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
	'5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
	'6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
	'7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
	'8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
	'9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
};

inline unsigned CountDecimalDigit32(uint32_t n) {
#if defined(_MSC_VER) || defined(__GNUC__)
	static const uint32_t powers_of_10[] = {
		0,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000
	};

#ifdef _MSC_VER
	unsigned long i = 0;
	_BitScanReverse(&i, n | 1);
	uint32_t t = (i + 1) * 1233 >> 12;
#elif __GNUC__
	uint32_t t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
#endif
	return t - (n < powers_of_10[t]) + 1;
#else
	// Simple pure C++ implementation
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n < 1000) return 3;
	if (n < 10000) return 4;
	if (n < 100000) return 5;
	if (n < 1000000) return 6;
	if (n < 10000000) return 7;
	if (n < 100000000) return 8;
	if (n < 1000000000) return 9;
	return 10;
#endif
}

inline unsigned CountDecimalDigit64(uint64_t n) {
#if defined(_MSC_VER) || defined(__GNUC__)
	static const uint64_t powers_of_10[] = {
		0,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000,
		10000000000,
		100000000000,
		1000000000000,
		10000000000000,
		100000000000000,
		1000000000000000,
		10000000000000000,
		100000000000000000,
		1000000000000000000,
		10000000000000000000U
	};
#if __GNUC__
	uint32_t t = (64 - __builtin_clzll(n | 1)) * 1233 >> 12;
#elif _M_IX86
	unsigned long i = 0;
	uint64_t m = n | 1;
	if (_BitScanReverse(&i, m >> 32))
		i += 32;
	else
		_BitScanReverse(&i, m & 0xFFFFFFFF);
	uint32_t t = (i + 1) * 1233 >> 12;
#elif _M_X64
	unsigned long i = 0;
	_BitScanReverse64(&i, n | 1);
	uint32_t t = (i + 1) * 1233 >> 12;
#endif
	return t - (n < powers_of_10[t]) + 1;
#else
	// Simple pure C++ implementation
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n < 1000) return 3;
	if (n < 10000) return 4;
	if (n < 100000) return 5;
	if (n < 1000000) return 6;
	if (n < 10000000) return 7;
	if (n < 100000000) return 8;
	if (n < 1000000000) return 9;
	if (n < 10000000000) return 10;
	if (n < 100000000000) return 11;
	if (n < 1000000000000) return 12;
	if (n < 10000000000000) return 13;
	if (n < 100000000000000) return 14;
	if (n < 1000000000000000) return 15;
	if (n < 10000000000000000) return 16;
	if (n < 100000000000000000) return 17;
	if (n < 1000000000000000000) return 18;
	if (n < 10000000000000000000) return 19;
	return 20;
#endif
}

inline void u32toa_countlut(uint32_t value, char* buffer) {
	unsigned digit = CountDecimalDigit32(value);
	buffer += digit;
	*buffer = '\0';

	while (value >= 100) {
		const unsigned i = (value % 100) << 1;
		value /= 100;
		*--buffer = gDigitsLut[i + 1];
		*--buffer = gDigitsLut[i];
	}

	if (value < 10) {
		*--buffer = char(value) + '0';
	}
	else {
		const unsigned i = value << 1;
		*--buffer = gDigitsLut[i + 1];
		*--buffer = gDigitsLut[i];
	}
}

inline void i32toa_countlut(int32_t value, char* buffer) {
	uint32_t u = static_cast<uint32_t>(value);
	if (value < 0) {
		*buffer++ = '-';
		u = ~u + 1;
	}
	u32toa_countlut(u, buffer);
}

inline void u64toa_countlut(uint64_t value, char* buffer) {
	unsigned digit = CountDecimalDigit64(value);
	buffer += digit;
	*buffer = '\0';

	while (value >= 100000000) {
		const uint32_t a = static_cast<uint32_t>(value % 100000000);
		value /= 100000000;

		const uint32_t b = a / 10000;
		const uint32_t c = a % 10000;

		const uint32_t b1 = (b / 100) << 1;
		const uint32_t b2 = (b % 100) << 1;
		const uint32_t c1 = (c / 100) << 1;
		const uint32_t c2 = (c % 100) << 1;

		buffer -= 8;

		buffer[0] = gDigitsLut[b1];
		buffer[1] = gDigitsLut[b1 + 1];
		buffer[2] = gDigitsLut[b2];
		buffer[3] = gDigitsLut[b2 + 1];
		buffer[4] = gDigitsLut[c1];
		buffer[5] = gDigitsLut[c1 + 1];
		buffer[6] = gDigitsLut[c2];
		buffer[7] = gDigitsLut[c2 + 1];
	}

	uint32_t value32 = static_cast<uint32_t>(value);
	while (value32 >= 100) {
		const unsigned i = static_cast<unsigned>(value32 % 100) << 1;
		value32 /= 100;
		*--buffer = gDigitsLut[i + 1];
		*--buffer = gDigitsLut[i];
	}

	if (value32 < 10) {
		*--buffer = char(value32) + '0';
	}
	else {
		const unsigned i = static_cast<unsigned>(value32) << 1;
		*--buffer = gDigitsLut[i + 1];
		*--buffer = gDigitsLut[i];
	}
}

inline void i64toa_countlut(int64_t value, char* buffer) {
	uint64_t u = static_cast<uint64_t>(value);
	if (value < 0) {
		*buffer++ = '-';
		u = ~u + 1;
	}
	u64toa_countlut(u, buffer);
}