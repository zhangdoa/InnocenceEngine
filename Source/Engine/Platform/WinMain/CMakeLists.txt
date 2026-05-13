include(${CMAKE_SOURCE_DIR}/CMake/DeployRuntimePayload.cmake)

add_executable(Main WIN32 WinMain.cpp)
target_sources(Main PRIVATE WinMain.rc)
set_target_properties(Main PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(Main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(Main Engine ExampleProjectFactory)
# CompileHlslShaders runs before Main links — POST_BUILD deploy then
# mirrors fresh DXIL into Bin/<Config>/Shaders/DXIL/ (TASK-202).
if(TARGET CompileHlslShaders)
  add_dependencies(Main CompileHlslShaders)
endif()
inno_deploy_runtime_payload(Main)

add_executable(RenderTest WIN32 WinMain.cpp)
target_sources(RenderTest PRIVATE WinMain.rc)
set_target_properties(RenderTest PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(RenderTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(RenderTest Engine TestClientFactory)
if(TARGET CompileHlslShaders)
  add_dependencies(RenderTest CompileHlslShaders)
endif()
inno_deploy_runtime_payload(RenderTest)
