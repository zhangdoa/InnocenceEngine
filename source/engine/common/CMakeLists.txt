add_library(InnoApplication InnoApplication.h InnoApplication.cpp)

if (INNO_PLATFORM_WIN)
add_executable(InnoMain WIN32 winMain.cpp)
else ()
add_executable(InnoMain main.cpp)
endif (INNO_PLATFORM_WIN)

target_link_libraries(InnoMain InnoApplication)
target_link_libraries(InnoMain InnoSystem)
target_link_libraries(InnoMain InnoGame)

if (INNO_PLATFORM_LINUX)
add_custom_target(run
    COMMAND InnoMain "-renderer GL"
    DEPENDS InnoMain
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif (INNO_PLATFORM_LINUX)
