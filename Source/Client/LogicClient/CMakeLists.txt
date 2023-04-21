file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultLogicClient SHARED ${HEADERS} ${SOURCES})
set_target_properties(DefaultLogicClient PROPERTIES FOLDER Client)
target_link_libraries(DefaultLogicClient Engine)