add_executable(Main LinuxMain.cpp)
set_target_properties(Main PROPERTIES FOLDER Engine/ApplicationEntry)
target_link_libraries(Main ApplicationEntry)

add_custom_target(run
    COMMAND Main "-renderer 0 -mode 0"
    DEPENDS Main
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)