file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDX12RenderingServer ${HEADERS} ${SOURCES})
set_target_properties(InnoDX12RenderingServer PROPERTIES FOLDER Engine/RenderingServer)