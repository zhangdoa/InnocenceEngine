file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(MTRenderingServer ${HEADERS} ${SOURCES})
set_target_properties(MTRenderingServer PROPERTIES FOLDER Engine/RenderingServer)