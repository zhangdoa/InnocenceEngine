add_subdirectory("ThirdParty")
add_subdirectory("Common")
add_subdirectory("System")

if(BUILD_GAME)
add_subdirectory("Platform")
endif (BUILD_GAME)