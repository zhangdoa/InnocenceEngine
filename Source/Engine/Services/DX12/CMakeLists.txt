file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_library(DX12GraphicsService ${HEADERS} ${SOURCES})
set_target_properties(DX12GraphicsService PROPERTIES FOLDER Engine/GraphicsService)
target_include_directories(DX12GraphicsService PUBLIC "${INNO_GITSUBMODULE_DIRECTORIES}/DirectX-Headers/include")

target_link_libraries(DX12GraphicsService PRIVATE d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib)
