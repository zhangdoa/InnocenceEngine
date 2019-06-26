add_subdirectory("ThirdParty")
add_subdirectory("Core")
add_subdirectory("FileSystem")
add_subdirectory("GameSystem")
add_subdirectory("PhysicsSystem")
add_subdirectory("RenderingFrontend")
add_subdirectory("RenderingBackend")
add_subdirectory("ImGuiWrapper")
add_subdirectory("RayTracer")

add_subdirectory("ModuleManager")

if(BUILD_GAME)
add_subdirectory("Platform")
endif (BUILD_GAME)

add_subdirectory("Application")