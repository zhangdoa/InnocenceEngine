aux_source_directory(. DIR_SRCS)
add_library(AssimpWrapper ${DIR_SRCS})
set_property(TARGET AssimpWrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(AssimpWrapper PROPERTIES FOLDER ThirdParty)

target_include_directories(AssimpWrapper PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/json/include")
target_include_directories(AssimpWrapper PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/assimp/include")
target_include_directories(AssimpWrapper PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/assimp/build/${CMAKE_BUILD_TYPE}/include")
target_include_directories(AssimpWrapper PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/stb")

target_include_directories(AssimpWrapper PRIVATE ${ASSIMP_INCLUDE_DIR})
foreach(LIB ${ASSIMP_LIBS})
    target_link_libraries(AssimpWrapper PRIVATE ${LIB})
endforeach()