add_executable(InnoMain main.cpp BaseApplication.h BaseApplication.cpp)

target_link_libraries(InnoMain InnoEntity)
target_link_libraries(InnoMain InnoComponent)
target_link_libraries(InnoMain InnoSystem)

link_directories(${CMAKE_SOURCE_DIR}/game/)

if (BUILD_EDITOR)
target_link_libraries(InnoMain InnoEditor)
endif (BUILD_EDITOR)

if(BUILD_GAME)
target_link_libraries(InnoMain InnoGame)
endif (BUILD_GAME)

if(BUILD_TEST)
target_link_libraries(InnoMain InnoTest)
endif (BUILD_TEST)

