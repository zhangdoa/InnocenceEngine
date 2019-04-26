file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoVKRenderingBackend ${HEADERS} ${SOURCES})
target_link_libraries(InnoVKRenderingBackend "${VULKAN}")