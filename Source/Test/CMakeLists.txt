add_executable(Test Test.cpp)
target_link_libraries(Test Core)
set_target_properties(Test PROPERTIES FOLDER Test)

if (INNO_PLATFORM_LINUX)
target_link_libraries(Test -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
