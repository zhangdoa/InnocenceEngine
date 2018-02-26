add_executable(InnoMain BaseApplicationInstance.cpp BaseApplication.h BaseApplication.cpp main.cpp)
target_link_libraries(InnoMain InnoCoreManager)
target_link_libraries(InnoMain InnoGraphicManager)
target_link_libraries(InnoMain InnoEntity)
target_link_libraries(InnoMain InnoComponent)

link_directories(${CMAKE_SOURCE_DIR}/game/)
target_link_libraries(InnoMain InnoGame)
