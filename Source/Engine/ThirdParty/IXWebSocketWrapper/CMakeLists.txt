set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)

add_subdirectory("../../../External/GitSubmodules/ixwebsocket" "ixwebsocket")

file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(IXWebSocketWrapper ${HEADERS} ${SOURCES})
set_property(TARGET IXWebSocketWrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(IXWebSocketWrapper PROPERTIES FOLDER Engine/ThirdParty)

target_link_libraries(IXWebSocketWrapper PUBLIC ixwebsocket)
