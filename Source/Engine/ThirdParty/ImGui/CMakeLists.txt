aux_source_directory(. DIR_SRCS)
list(REMOVE_ITEM DIR_SRCS ./imgui_impl_win32.h ./imgui_impl_dx11.h ./imgui_impl_opengl3.h ./imgui_impl_win32.cpp ./imgui_impl_dx11.cpp ./imgui_impl_opengl3.cpp)
add_library(ImGui ${DIR_SRCS})
set_property(TARGET ImGui PROPERTY POSITION_INDEPENDENT_CODE ON)