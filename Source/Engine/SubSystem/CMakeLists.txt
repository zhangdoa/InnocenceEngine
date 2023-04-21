file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(SubSystem ${HEADERS} ${SOURCES})
set_property(TARGET SubSystem PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(SubSystem PROPERTIES FOLDER Engine)
target_include_directories(SubSystem PRIVATE "${INNO_SUBMODULE_DIRECTORIES}/json/include")

target_link_libraries(SubSystem AssimpWrapper)
target_link_libraries(SubSystem STBWrapper)
target_link_libraries(SubSystem JSONWrapper)

if (INNO_PLATFORM_WIN)
target_link_libraries(SubSystem PhysXWrapper)
endif (INNO_PLATFORM_WIN)