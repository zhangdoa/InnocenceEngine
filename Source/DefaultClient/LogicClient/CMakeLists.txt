file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultLogicClient ${HEADERS} ${SOURCES})
set_target_properties(DefaultLogicClient PROPERTIES FOLDER DefaultClient)
target_link_libraries(DefaultLogicClient Engine)