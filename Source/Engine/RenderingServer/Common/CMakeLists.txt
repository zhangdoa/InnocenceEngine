file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(RenderingServerCommon ${HEADERS} ${SOURCES})
set_property(TARGET RenderingServerCommon PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(RenderingServerCommon PROPERTIES FOLDER Engine/RenderingServer)