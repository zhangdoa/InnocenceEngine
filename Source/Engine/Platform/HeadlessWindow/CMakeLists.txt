file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(HeadlessWindowSystem ${HEADERS} ${SOURCES})
set_target_properties(HeadlessWindowSystem PROPERTIES FOLDER Engine/Window)
