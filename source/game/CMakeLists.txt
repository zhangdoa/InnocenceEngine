option (BUILD_EDITOR "Build editor" OFF)
if (BUILD_EDITOR)
add_subdirectory("InnocenceEditor")
endif (BUILD_EDITOR)

option (BUILD_GAME "Build game" OFF)
if(BUILD_GAME)
add_subdirectory("InnocenceGarden")
endif (BUILD_GAME)

option (BUILD_TEST "Build test" ON)
if(BUILD_TEST)
add_subdirectory("InnocenceTest")
endif (BUILD_TEST)
