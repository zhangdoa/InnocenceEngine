file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(HeadlessWindowService ${HEADERS} ${SOURCES})
set_target_properties(HeadlessWindowService PROPERTIES FOLDER Engine/Window)
