if (NOT INNO_PLATFORM_MAC)
add_executable(GLSLParser GLSLParser.cpp)
set_target_properties(GLSLParser PROPERTIES FOLDER Tool)
target_link_libraries(GLSLParser Core)
if (INNO_PLATFORM_LINUX)
target_link_libraries(GLSLParser -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
endif (NOT INNO_PLATFORM_MAC)