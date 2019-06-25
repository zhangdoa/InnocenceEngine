add_subdirectory("ThirdParty")
add_subdirectory("Core")
add_subdirectory("FileSystem")
add_subdirectory("GameSystem")
add_subdirectory("PhysicsSystem")
add_subdirectory("RenderingFrontend")
add_subdirectory("RenderingBackend")
add_subdirectory("ImGuiWrapper")
add_subdirectory("RayTracer")

if (INNO_PLATFORM_WIN)
add_subdirectory("WinWindow")
endif (INNO_PLATFORM_WIN)

if (INNO_PLATFORM_MAC)
add_subdirectory("MacWindow")
endif (INNO_PLATFORM_MAC)

if (INNO_PLATFORM_LINUX)
add_subdirectory("LinuxWindow")
endif (INNO_PLATFORM_LINUX)

add_subdirectory("ModuleManager")

if(BUILD_GAME)
add_subdirectory("Platform")
endif (BUILD_GAME)

add_subdirectory("Application")