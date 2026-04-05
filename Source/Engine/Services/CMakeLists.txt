file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(Services ${HEADERS} ${SOURCES})
set_property(TARGET Services PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(Services PROPERTIES FOLDER Engine)
target_include_directories(Services PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/json/include")

target_link_libraries(Services AssimpWrapper)
target_link_libraries(Services STBWrapper)
target_link_libraries(Services JSONWrapper)
target_link_libraries(Services ImGuiWrapper)
target_link_libraries(Services IXWebSocketWrapper)

if (INNO_PLATFORM_WIN)
target_link_libraries(Services PhysXWrapper)
endif (INNO_PLATFORM_WIN)

add_subdirectory("Common")

if (INNO_RENDERER_DIRECTX)
add_subdirectory("DX12")
endif (INNO_RENDERER_DIRECTX)

if (INNO_RENDERER_VULKAN)
add_subdirectory("VK")
endif (INNO_RENDERER_VULKAN)

if (INNO_RENDERER_METAL)
add_subdirectory("MT")
endif (INNO_RENDERER_METAL)