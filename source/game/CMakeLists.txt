if (BUILD_EDITOR)
add_subdirectory("InnocenceEditor")
endif (BUILD_EDITOR)

if(BUILD_GAME)
add_subdirectory("InnocenceGarden")
endif (BUILD_GAME)

if(BUILD_TEST)
add_subdirectory("InnocenceTest")
endif (BUILD_TEST)
