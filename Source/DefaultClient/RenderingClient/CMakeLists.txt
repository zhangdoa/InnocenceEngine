file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultRenderingClient ${HEADERS} ${SOURCES})
set_target_properties(DefaultRenderingClient PROPERTIES FOLDER DefaultClient)
target_link_libraries(DefaultRenderingClient Engine)