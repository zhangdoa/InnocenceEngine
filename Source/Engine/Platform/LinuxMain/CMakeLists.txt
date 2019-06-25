add_executable(InnoMain LinuxMain.cpp)

target_link_libraries(InnoMain InnoApplication)

add_custom_target(run
    COMMAND InnoMain "-renderer 0 -mode 0"
    DEPENDS InnoMain
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
