file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(InnoSubSystem ${HEADERS} ${SOURCES})
set_property(TARGET InnoSubSystem PROPERTY POSITION_INDEPENDENT_CODE ON)

if (INNO_PLATFORM_WIN)
target_link_libraries(InnoSubSystem PhysXWrapper)
endif (INNO_PLATFORM_WIN)