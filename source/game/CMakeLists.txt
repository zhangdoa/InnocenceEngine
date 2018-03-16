option (BUILD_EDITOR "Build editor" OFF)
if (BUILD_EDITOR)
add_subdirectory("InnocenceEditor")
endif (BUILD_EDITOR)

option (BUILD_GAME "Build game" ON)
if(BUILD_GAME)
add_subdirectory("InnocenceGarden")
endif (BUILD_GAME)


