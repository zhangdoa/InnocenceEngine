file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(InnoDefaultLogicClient SHARED ${HEADERS} ${SOURCES})
target_link_libraries(InnoDefaultLogicClient InnoEngine)