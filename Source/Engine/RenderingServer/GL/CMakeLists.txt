file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoGLRenderingServer ${HEADERS} ${SOURCES})
set_property(TARGET InnoGLRenderingServer PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(InnoGLRenderingServer PROPERTIES FOLDER Engine/RenderingServer)
target_link_libraries(InnoGLRenderingServer ${OPENGL_LIBRARIES})
# Path to glad directory
set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/External/GitSubmodules/GLAD/")
# Path to glad cmake files
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)

# Specify glad settings
glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:compatibility=4.6)
target_link_libraries(InnoGLRenderingServer glad_gl_core_46)