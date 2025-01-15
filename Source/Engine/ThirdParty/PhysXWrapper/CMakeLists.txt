add_library(PhysXWrapper PhysXWrapper.h PhysXWrapper.cpp)
set_property(TARGET PhysXWrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(PhysXWrapper PROPERTIES FOLDER ThirdParty)
target_include_directories(PhysXWrapper PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/PhysX/physx/include")

target_link_libraries(PhysXWrapper PRIVATE ${PHYSX_LIBS})