file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(${INNO_LOGIC_CLIENT} ${HEADERS} ${SOURCES})
set_target_properties(${INNO_LOGIC_CLIENT} PROPERTIES FOLDER Client)
target_link_libraries(${INNO_LOGIC_CLIENT} Engine)