file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoMTRenderingServer ${HEADERS} ${SOURCES})
set_target_properties(InnoMTRenderingServer PROPERTIES FOLDER Engine/RenderingServer)