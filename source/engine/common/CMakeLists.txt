add_executable(InnoMain main.cpp BaseApplication.h BaseApplication.cpp)

target_link_libraries(InnoMain InnoEntity)
target_link_libraries(InnoMain InnoComponent)
target_link_libraries(InnoMain InnoSystem)

link_directories(${CMAKE_SOURCE_DIR}/game/)
target_link_libraries(InnoMain InnoGame)
