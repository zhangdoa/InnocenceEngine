file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(ExampleRenderingClient ${HEADERS} ${SOURCES})
set_target_properties(ExampleRenderingClient PROPERTIES FOLDER ExampleProject)
target_link_libraries(ExampleRenderingClient Engine)

# Propagate the root BUILD_WITH_NRD option as a compile definition so
# NRDConstants.h can constexpr-branch without #including any NRD header.
# The define is added ONLY when ON — the OFF path adds nothing to the compile
# command line, which is what AC-3 of TASK-77.4 demands ("OFF-build binary
# identical to pre-CL baseline"). NRDConstants.h treats absent INNO_BUILD_WITH_NRD
# as the OFF state. PRIVATE rather than PUBLIC: only RenderingClient TUs use
# the constexpr; consumers of the lib (Main.exe) must not see the define on
# their command line, again to keep OFF-builds bit-identical.
if(BUILD_WITH_NRD)
    target_compile_definitions(ExampleRenderingClient PRIVATE INNO_BUILD_WITH_NRD=1)
endif()