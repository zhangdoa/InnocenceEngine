add_subdirectory("ThirdParty")
add_subdirectory("Core")
add_subdirectory("SubSystem")
add_subdirectory("RenderingFrontend")
add_subdirectory("RenderingServer")
add_subdirectory("RayTracer")

add_subdirectory("EntityManager")
add_subdirectory("ComponentManager")
add_subdirectory("ModuleManager")

if(BUILD_GAME)
add_subdirectory("Platform")
endif (BUILD_GAME)