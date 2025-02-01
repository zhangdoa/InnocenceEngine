file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(${INNO_RENDERING_CLIENT} ${HEADERS} ${SOURCES})
set_target_properties(${INNO_RENDERING_CLIENT} PROPERTIES FOLDER Client)