file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoVKRenderingServer ${HEADERS} ${SOURCES})
set_property(TARGET InnoVKRenderingServer PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(InnoVKRenderingServer PROPERTIES FOLDER Engine/RenderingServer)
target_link_libraries(InnoVKRenderingServer ${Vulkan_LIBRARIES})