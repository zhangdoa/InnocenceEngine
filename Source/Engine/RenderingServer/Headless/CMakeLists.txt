file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(HeadlessRenderingServer ${HEADERS} ${SOURCES})
set_target_properties(HeadlessRenderingServer PROPERTIES FOLDER Engine/RenderingServer)
