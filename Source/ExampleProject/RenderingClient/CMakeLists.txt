file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(ExampleRenderingClient ${HEADERS} ${SOURCES})
set_target_properties(ExampleRenderingClient PROPERTIES FOLDER ExampleProject)
target_link_libraries(ExampleRenderingClient Engine)