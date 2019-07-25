file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoClient SHARED ${HEADERS} ${SOURCES})

link_directories(${CMAKE_SOURCE_DIR}/Engine/System/)
target_link_libraries(InnoClient InnoEngine)
