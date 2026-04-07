file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(ExampleLogicClient ${HEADERS} ${SOURCES})
set_target_properties(ExampleLogicClient PROPERTIES FOLDER ExampleProject)
target_link_libraries(ExampleLogicClient Engine)