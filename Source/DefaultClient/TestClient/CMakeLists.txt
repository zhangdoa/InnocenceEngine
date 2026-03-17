file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(TestClient ${HEADERS} ${SOURCES})
set_target_properties(TestClient PROPERTIES FOLDER Client)
