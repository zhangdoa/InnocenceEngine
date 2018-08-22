if (INNO_RENDERER_DX)
add_executable(InnoMain WIN32 main.cpp BaseApplication.h BaseApplication.cpp)
else ()
add_executable(InnoMain main.cpp BaseApplication.h BaseApplication.cpp)
endif (INNO_RENDERER_DX)

target_link_libraries(InnoMain InnoSystem)

