# DeployRuntimePayload.cmake
#
# Replaces the manual NTFS-junction workaround for getting compiled shaders and
# Data/* asset trees next to a built executable. Hooks POST_BUILD steps onto each
# runtime target that mirror two payload roots into the per-config Bin output:
#
#   Bin/Shaders/DXIL/  ->  Bin/<Config>/Shaders/DXIL/
#   Data/<sub>/        ->  Bin/Data/<sub>/                (config-independent)
#
# Engine path resolution at runtime (Source/Engine/Common/IOService.cpp):
#   m_workingDir = current_path()                e.g. Bin/RelWithDebInfo/
#   m_dataDir    = m_workingDir + "../Data/"     e.g. Bin/Data/
#   shaders      = m_workingDir + "Shaders/DXIL/<name>.dxil"
#
# So Bin/<Config>/Shaders/ must hold the DXIL outputs and Bin/Data/<sub>/ must
# hold the source-tree asset roots. CMake's `-E copy_directory` is used because
# it is cross-platform (no NTFS-only junctions / Win32-only mklink) and
# idempotent — re-running the build does not error if the destinations already
# exist. The cost is a few MB of duplicated content per built Config; the
# benefit is a runnable Bin/<Config>/ from a fresh clone with no manual setup.

function(inno_deploy_runtime_payload target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "inno_deploy_runtime_payload: '${target}' is not a target")
    endif()

    set(_shaders_src "${CMAKE_SOURCE_DIR}/Bin/Shaders")
    set(_data_root_src "${CMAKE_SOURCE_DIR}/Data")
    set(_data_root_dst "${CMAKE_SOURCE_DIR}/Bin/Data")

    # Per-config shader deploy — $<TARGET_FILE_DIR:...> evaluates per Config at
    # build time, so the Visual Studio multi-config generator picks the right
    # destination (Bin/Debug, Bin/Release, Bin/RelWithDebInfo).
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_shaders_src}/DXIL"
            "$<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        COMMENT "Deploying DXIL shaders to $<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        VERBATIM
    )

    # Data trees — the engine resolves m_dataDir as `<cwd>/../Data`, which from
    # Bin/<Config>/ is the single shared Bin/Data/. So we mirror once into
    # Bin/Data/<sub>; every Config sees the same payload. Subdirs that do not
    # exist in the source tree on this checkout (e.g. local-only `Components/`
    # or `UnitTest/` on a stripped clone) are skipped — the engine tolerates a
    # missing project subdir at boot but a missing Engine/ root would fail, so
    # that one is asserted at configure time. Engine/ holds boot-required
    # tracked assets (e.g. Engine/Fonts/FreeSans.otf consumed by
    # ImGuiWrapper::Initialize). `Generated/` is purely runtime-derived
    # (Components/ ~1.2 GB textures+meshes, Scenes/ serialized scene state)
    # and gitignored; the deploy mirrors it when locally present so dev-box
    # iteration sees fresh derived output, but it is absent on fresh clones
    # by design.
    set(_required_subdirs Engine)
    set(_optional_subdirs ExampleProject UnitTest Components Generated)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_data_root_dst}"
        COMMENT "Ensuring Bin/Data/ exists"
        VERBATIM
    )

    foreach(_sub IN LISTS _required_subdirs)
        set(_src "${_data_root_src}/${_sub}")
        if(NOT IS_DIRECTORY "${_src}")
            message(FATAL_ERROR
                "inno_deploy_runtime_payload: required Data subdir missing: ${_src}")
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_src}" "${_data_root_dst}/${_sub}"
            COMMENT "Deploying Data/${_sub} -> Bin/Data/${_sub}"
            VERBATIM
        )
    endforeach()

    foreach(_sub IN LISTS _optional_subdirs)
        set(_src "${_data_root_src}/${_sub}")
        if(IS_DIRECTORY "${_src}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_src}" "${_data_root_dst}/${_sub}"
                COMMENT "Deploying Data/${_sub} -> Bin/Data/${_sub}"
                VERBATIM
            )
        else()
            message(STATUS
                "inno_deploy_runtime_payload: optional Data/${_sub} not present; skipping deploy for ${target}")
        endif()
    endforeach()
endfunction()
