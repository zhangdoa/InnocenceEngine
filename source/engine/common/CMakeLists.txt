if (INNO_PLATFORM_WIN)
add_executable(InnoMain WIN32 main.cpp InnoApplication.h InnoApplication.cpp)
else ()
add_executable(InnoMain main.cpp InnoApplication.h InnoApplication.cpp)
endif (INNO_PLATFORM_WIN)

target_link_libraries(InnoMain InnoSystem)
target_link_libraries(InnoMain InnoGame)

if (INNO_PLATFORM_LINUX)
add_custom_target(run
    COMMAND InnoMain "-renderer GL"
    DEPENDS InnoMain
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif (INNO_PLATFORM_LINUX)
