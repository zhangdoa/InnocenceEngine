add_subdirectory("third-party")
add_subdirectory("common")
add_subdirectory("system")

if(BUILD_GAME)
add_subdirectory("platform")
endif (BUILD_GAME)