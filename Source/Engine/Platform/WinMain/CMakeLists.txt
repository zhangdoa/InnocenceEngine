add_executable(Main WIN32 WinMain.cpp)
target_sources(Main PRIVATE WinMain.rc)
set_target_properties(Main PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(Main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(Main Engine ExampleProjectFactory)

add_executable(RenderTest WIN32 WinMain.cpp)
target_sources(RenderTest PRIVATE WinMain.rc)
set_target_properties(RenderTest PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(RenderTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(RenderTest Engine TestClientFactory)