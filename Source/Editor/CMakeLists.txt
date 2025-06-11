# Qt Editor CMake Configuration
cmake_minimum_required(VERSION 3.16)

# Try to find Qt6 in common locations
if(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    # Common Qt installation paths
    set(QT_SEARCH_PATHS
        "C:/Qt/6.8.3/msvc2022_64"
        "C:/Qt/6.7.*/msvc2022_64" 
        "C:/Qt/6.6.*/msvc2022_64"
        "C:/Qt/6.5.*/msvc2022_64"
        "$ENV{QT_DIR}"
        "$ENV{QT_ROOT}/6.8.3/msvc2022_64"
        "$ENV{QT_ROOT}/6.7.*/msvc2022_64"
    )
    
    foreach(QT_PATH ${QT_SEARCH_PATHS})
        if(EXISTS "${QT_PATH}/lib/cmake/Qt6")
            set(CMAKE_PREFIX_PATH "${QT_PATH}" CACHE PATH "Qt installation path" FORCE)
            message(STATUS "Found Qt6 at: ${QT_PATH}")
            break()
        endif()
    endforeach()
endif()

# Find Qt6 components
find_package(Qt6 QUIET COMPONENTS Core Widgets Gui)

if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    message(STATUS "Qt6 found - building Qt Editor")
    
    # Collect source files automatically (engine compilation issues fixed!)
    file(GLOB EDITOR_SOURCES "*.cpp")
    file(GLOB EDITOR_HEADERS "*.h")
    file(GLOB EDITOR_UI_FILES "*.ui")
    file(GLOB EDITOR_QRC_FILES "*.qrc")
    
    # Include theme resources
    file(GLOB THEME_QRC_FILES "themes/*.qrc")
    
    # Combine all sources
    set(ALL_EDITOR_FILES 
        ${EDITOR_SOURCES}
        ${EDITOR_HEADERS}
        ${EDITOR_UI_FILES}
        ${EDITOR_QRC_FILES}
        ${THEME_QRC_FILES}
    )
    
    add_executable(Editor ${ALL_EDITOR_FILES})
    
    # Set C++17 standard
    target_compile_features(Editor PRIVATE cxx_std_17)
    
    # Link Qt libraries
    target_link_libraries(Editor Qt6::Core Qt6::Widgets Qt6::Gui)
    
    # Link engine libraries
    target_link_libraries(Editor 
        Engine
        DefaultRenderingClient
        DefaultLogicClient
    )
    
    # Include directories
    target_include_directories(Editor PRIVATE 
        ${CMAKE_SOURCE_DIR}/Source/External/Include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Set output directory
    set_target_properties(Editor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/Bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/Bin/Release  
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/Bin/RelWithDebInfo
    )
    
    # Windows-specific settings
    if(WIN32)
        set_target_properties(Editor PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
    
    # Copy Qt DLLs to output directory (Windows)
    if(WIN32)
        add_custom_command(TARGET Editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:Editor>
        )
        
        # Copy Qt platform plugins
        add_custom_command(TARGET Editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory 
            $<TARGET_FILE_DIR:Editor>/platforms
        )
        add_custom_command(TARGET Editor POST_BUILD  
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Qt6_DIR}/../../../plugins/platforms/qwindows.dll
            $<TARGET_FILE_DIR:Editor>/platforms/
        )
    endif()
    
else()
    message(WARNING "Qt6 not found - Qt Editor will not be built. Install Qt6 to enable editor development.")
    message(STATUS "Try: vcpkg install qt6[core,widgets,tools]:x64-windows")
endif()
