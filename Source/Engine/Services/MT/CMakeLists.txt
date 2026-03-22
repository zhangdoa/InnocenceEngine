file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(MTGraphicsService ${HEADERS} ${SOURCES})
set_target_properties(MTGraphicsService PROPERTIES FOLDER Engine/GraphicsService)