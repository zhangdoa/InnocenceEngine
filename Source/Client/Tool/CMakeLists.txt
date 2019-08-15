add_executable(InnoGLSLParser GLSLParser.cpp)
target_link_libraries(InnoGLSLParser InnoCore)

if (INNO_PLATFORM_WIN)
add_executable(InnoBaker WIN32 InnoBaker.h InnoBaker.cpp InnoBakerMain.cpp)
else ()
add_executable(InnoBaker InnoBaker.h InnoBaker.cpp InnoBakerMain.cpp)
endif (INNO_PLATFORM_WIN)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(InnoBaker PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(InnoBaker InnoEngine)
target_link_libraries(InnoBaker InnoDefaultGPUBuffers)