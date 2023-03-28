file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoSubSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoSubSystem PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(InnoSubSystem PROPERTIES FOLDER Engine)
target_include_directories(InnoSubSystem PRIVATE "${INNO_INCLUDE_DIRECTORIES}/json")

target_link_libraries(InnoSubSystem AssimpWrapper)
target_link_libraries(InnoSubSystem STBWrapper)
target_link_libraries(InnoSubSystem JSONWrapper)

if (INNO_PLATFORM_WIN)
target_link_libraries(InnoSubSystem PhysXWrapper)
endif (INNO_PLATFORM_WIN)