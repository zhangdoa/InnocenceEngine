file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoDefaultLogicClient SHARED ${HEADERS} ${SOURCES})

link_directories(${CMAKE_SOURCE_DIR}/Engine/System/)
target_link_libraries(InnoDefaultLogicClient InnoEngine)
