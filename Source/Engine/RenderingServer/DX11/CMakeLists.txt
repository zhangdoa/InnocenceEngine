file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(DX11RenderingServer ${HEADERS} ${SOURCES})
set_target_properties(DX11RenderingServer PROPERTIES FOLDER Engine/RenderingServer)