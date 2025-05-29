if (INNO_PLATFORM_WIN)
add_subdirectory("WinMain")
add_subdirectory("WinWindow")
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_MAC)
add_subdirectory("MacMain")
add_subdirectory("MacWindow")
endif (INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
add_subdirectory("LinuxMain")
add_subdirectory("LinuxWindow")
endif (INNO_PLATFORM_LINUX)

# Always include headless window system (platform-agnostic)
add_subdirectory("HeadlessWindow")