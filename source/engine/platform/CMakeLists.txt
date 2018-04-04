option (INNO_PLATFORM_WIN32 "Windows x86 32-bit" OFF)
option (INNO_PLATFORM_WIN64 "Windows x86-64 64-bit" ON)

if (INNO_PLATFORM_WIN32)
include(InnoSystemHeaderWin32.h)
endif (INNO_PLATFORM_WIN32)

if (INNO_PLATFORM_WIN64)
include(InnoSystemHeaderWin64.h)
endif (INNO_PLATFORM_WIN64)

