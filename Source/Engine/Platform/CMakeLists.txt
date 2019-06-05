if (INNO_PLATFORM_WIN)
add_subdirectory("WinMain")
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_MAC)
endif (INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
add_subdirectory("LinuxMain")
endif (INNO_PLATFORM_LINUX)