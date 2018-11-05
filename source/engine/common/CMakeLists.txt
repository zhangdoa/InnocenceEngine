if (INNO_PLATFORM_WIN32 OR INNO_PLATFORM_WIN64)
add_executable(InnoMain WIN32 main.cpp InnoApplication.h InnoApplication.cpp)
else ()
add_executable(InnoMain main.cpp InnoApplication.h InnoApplication.cpp)
endif (INNO_PLATFORM_WIN32 OR INNO_PLATFORM_WIN64)

target_link_libraries(InnoMain InnoSystem)