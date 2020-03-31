add_executable(InnoMain WIN32 WinMain.cpp)
set_target_properties(InnoMain PROPERTIES FOLDER Engine/ApplicationEntry)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(InnoMain PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(InnoMain InnoApplicationEntry)