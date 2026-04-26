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
# hold the source-tree asset roots.
#
# MIRROR SEMANTICS — TASK-146
# ----------------------------
# Each deploy step is mirror-semantic: the destination is wiped (`cmake -E rm
# -rRf`) before being repopulated (`cmake -E copy_directory`). This is required
# because plain `copy_directory` is additive — files removed from the source
# (deleted shader, renamed asset, branch switch, bisect step) linger in the
# destination and can be loaded by the engine, producing binding mismatches and
# device-removed errors that look like rendering regressions but are actually
# stale-cache contamination. The TASK-141..145 phantom regression chain was
# entirely caused by this; see TASK-146 for the incident write-up.
#
# Cost: each POST_BUILD wipes-and-recopies a few MB of DXIL plus ~100 MB of
# Data/Engine. Wall time on a warm SSD is sub-second per target. The
# Generated/ subtree is gitignored runtime-derived output (~1 GB+) and is
# intentionally NOT mirrored on every build — see _generated_subdirs handling
# below.

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
    #
    # Mirror semantics: rm -rRf the destination DXIL dir first, then copy. This
    # ensures shaders deleted from Bin/Shaders/DXIL by the upstream
    # orphan-removal pass (Scripts/Lib/Compile-HLSL.psm1) propagate into the
    # per-Config deploy target on the next build.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rRf
            "$<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_shaders_src}/DXIL"
            "$<TARGET_FILE_DIR:${target}>/Shaders/DXIL"
        COMMENT "Mirror-deploying DXIL shaders -> $<TARGET_FILE_DIR:${target}>/Shaders/DXIL (wipe + copy)"
        VERBATIM
    )

    # Data trees — the engine resolves m_dataDir as `<cwd>/../Data`, which from
    # Bin/<Config>/ is the single shared Bin/Data/. So we mirror once into
    # Bin/Data/<sub>; every Config sees the same payload. Engine/ holds
    # boot-required tracked assets (e.g. Engine/Fonts/FreeSans.otf consumed by
    # ImGuiWrapper::Initialize) and is asserted at configure time.
    set(_required_subdirs Engine)

    # Optional, mirror-semantic: present in source tree -> wiped+copied to
    # Bin/Data/<sub>/. Absent -> the Bin/Data/<sub>/ destination is also wiped
    # so a removed source subdir does not leave a stale Bin/Data tree behind.
    set(_optional_subdirs ExampleProject UnitTest Components)

    # Generated/ is gitignored runtime-derived output (Components/ ~1.2 GB
    # textures+meshes, Scenes/ serialized scene state). Wiping-and-recopying
    # on every build would be wasteful (large) and unsafe (engine may be
    # reading/writing it). We mirror on first creation only — `make_directory`
    # is idempotent and `copy_directory` is additive. If a subdirectory of
    # Generated/ ever needs orphan removal, that should be an explicit clean
    # target, not part of every POST_BUILD. See TASK-146 implementation notes.
    set(_generated_subdirs Generated)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_data_root_dst}"
        COMMENT "Ensuring Bin/Data/ exists"
        VERBATIM
    )

    foreach(_sub IN LISTS _required_subdirs)
        set(_src "${_data_root_src}/${_sub}")
        set(_dst "${_data_root_dst}/${_sub}")
        if(NOT IS_DIRECTORY "${_src}")
            message(FATAL_ERROR
                "inno_deploy_runtime_payload: required Data subdir missing: ${_src}")
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rRf "${_dst}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${_src}" "${_dst}"
            COMMENT "Mirror-deploying Data/${_sub} -> Bin/Data/${_sub} (wipe + copy)"
            VERBATIM
        )
    endforeach()

    foreach(_sub IN LISTS _optional_subdirs)
        set(_src "${_data_root_src}/${_sub}")
        set(_dst "${_data_root_dst}/${_sub}")
        if(IS_DIRECTORY "${_src}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rRf "${_dst}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_src}" "${_dst}"
                COMMENT "Mirror-deploying Data/${_sub} -> Bin/Data/${_sub} (wipe + copy)"
                VERBATIM
            )
        else()
            # Source subdir absent — wipe any stale Bin/Data/<sub> so an old
            # deploy of a since-removed asset tree does not contaminate the
            # runtime payload. Mirror semantics extends to "absent" too.
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rRf "${_dst}"
                COMMENT "Mirror-deploying Data/${_sub}: source absent, wiping Bin/Data/${_sub}"
                VERBATIM
            )
        endif()
    endforeach()

    # Generated/: additive only (see rationale above). Created if absent so
    # downstream code can write into it; never wiped automatically.
    foreach(_sub IN LISTS _generated_subdirs)
        set(_src "${_data_root_src}/${_sub}")
        set(_dst "${_data_root_dst}/${_sub}")
        if(IS_DIRECTORY "${_src}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_src}" "${_dst}"
                COMMENT "Additive-deploying Data/${_sub} -> Bin/Data/${_sub} (Generated/, no wipe)"
                VERBATIM
            )
        endif()
    endforeach()
endfunction()
