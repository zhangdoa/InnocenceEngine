#pragma once
#include "Config.h"
#include "Metadata.h"
#include "STL14.h"

#if defined INNO_PLATFORM_WIN
#define INNO_FORCEINLINE __forceinline
#else
#define INNO_FORCEINLINE __attribute__((always_inline)) inline
#endif

#define INNO_ENUM_OPERATORS(enumTypeName)      \
inline enumTypeName operator&(enumTypeName a, enumTypeName b) \
{ \
	return static_cast<enumTypeName>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); \
} \
\
inline enumTypeName& operator&=(enumTypeName& a, enumTypeName b) \
{ \
	a = static_cast<enumTypeName>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); \
	return a; \
} \
\
inline enumTypeName operator|(enumTypeName a, enumTypeName b) \
{ \
	return static_cast<enumTypeName>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); \
} \
\
inline enumTypeName& operator|=(enumTypeName& a, enumTypeName b) \
{ \
	a = static_cast<enumTypeName>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); \
	return a; \
}