file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(InnoDefaultLogicClient SHARED ${HEADERS} ${SOURCES})
set_target_properties(InnoDefaultLogicClient PROPERTIES FOLDER Client)
target_link_libraries(InnoDefaultLogicClient InnoEngine)