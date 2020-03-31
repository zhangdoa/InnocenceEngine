file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDX11RenderingServer ${HEADERS} ${SOURCES})
set_target_properties(InnoDX11RenderingServer PROPERTIES FOLDER Engine/RenderingServer)