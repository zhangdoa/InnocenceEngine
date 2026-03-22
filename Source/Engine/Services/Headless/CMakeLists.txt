file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(HeadlessGraphicsService ${HEADERS} ${SOURCES})
set_target_properties(HeadlessGraphicsService PROPERTIES FOLDER Engine/GraphicsService)
