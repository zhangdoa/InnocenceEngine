add_executable(InnoMain LinuxMain.cpp)
set_target_properties(InnoMain PROPERTIES FOLDER Engine/ApplicationEntry)
target_link_libraries(InnoMain InnoApplicationEntry)

add_custom_target(run
    COMMAND InnoMain "-renderer 0 -mode 0"
    DEPENDS InnoMain
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)