cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)

project("vulkan-cpp-tutorial")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DE_PACKAGE_RELEASE "Enable Windows Release packaging" OFF)

find_program(GLSLC_EXECUTABLE glslc)

# ---- Внешние зависимости через FetchContent — без conan ----
include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # 1.0.3
)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # 3.4
)
# GLFW не должен собирать свои примеры/тесты/докладчики — нам нужна только сама библиотека
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# tinyobjloader — header-only библиотека для загрузки .obj моделей (DE_Model.cpp)
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG          45636bdcef1a4fec140346b90c0b50bf0bc3e23b #2026-06-19
)

FetchContent_MakeAvailable(glm glfw tinyobjloader)

if(NOT DEFINED VULKAN_SDK_ROOT)
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        if(NOT "${Vulkan_INCLUDE_DIRS}" STREQUAL "")
            set(VULKAN_SDK_ROOT ${Vulkan_INCLUDE_DIRS})
            string(REGEX REPLACE "/Include" "" VULKAN_SDK_ROOT ${VULKAN_SDK_ROOT})
        endif()
        message(STATUS "Found Vulkan SDK at ${VULKAN_SDK_ROOT}")
    else()
        message(FATAL_ERROR "Vulkan SDK not found. Set VULKAN_SDK_ROOT.")
    endif()
else()
    if(WIN32)
        set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_ROOT}/Include")
        set(Vulkan_LIBRARIES    "${VULKAN_SDK_ROOT}/lib/vulkan-1.lib")
    else()
        set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_ROOT}/include")
        set(Vulkan_LIBRARIES    "${VULKAN_SDK_ROOT}/lib/libvulkan.dylib")
    endif()
endif()

# ---- Автопоиск всех .cpp и .hpp файлов в src (включая подпапки Systems/, Utility/) ----
# Заголовки тоже добавляются в таргет (хоть и не компилируются отдельно) —
# иначе Visual Studio их вообще не увидит в Solution Explorer и не сможет
# построить для них фильтры.
file(GLOB_RECURSE VULKAN_TUTORIAL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE VULKAN_TUTORIAL_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

add_executable(vulkan_tutorial ${VULKAN_TUTORIAL_SOURCES} ${VULKAN_TUTORIAL_HEADERS})

# ---- Автоматическая генерация фильтров Solution Explorer по реальной структуре папок ----
# Без этого VS/CMake кладёт все .cpp в один плоский "Source Files" и не видит подпапки
# (Systems/, Utility/). TREE-режим source_group строит дерево фильтров 1-в-1 с файловой системой.
source_group(TREE "${CMAKE_SOURCE_DIR}/src" FILES ${VULKAN_TUTORIAL_SOURCES} ${VULKAN_TUTORIAL_HEADERS})

target_include_directories(vulkan_tutorial PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/third_party/imgui
    ${Vulkan_INCLUDE_DIRS}
)

# ---- Шейдеры лежат в Shaders/ на уровне корня проекта, рядом с Models/ ----
# Автопоиск: компилируется любой файл с расширением, которое понимает glslc,
# независимо от имени — не нужно перечислять каждый шейдер вручную.
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Shaders")
if(DE_PACKAGE_RELEASE)
    # Keep packaged-build shader output separate from the Debug build.
    set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/de-compiled-shaders")
else()
    set(SHADER_BINARY_DIR "${SHADER_SOURCE_DIR}/bin")
endif()

file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
    "${SHADER_SOURCE_DIR}/*.geom"
    "${SHADER_SOURCE_DIR}/*.tesc"
    "${SHADER_SOURCE_DIR}/*.tese"
    "${SHADER_SOURCE_DIR}/*.rgen"
    "${SHADER_SOURCE_DIR}/*.rmiss"
    "${SHADER_SOURCE_DIR}/*.rchit"
)

# Показываем исходники шейдеров в Solution Explorer как часть таргета,
# сгруппированные в отдельный фильтр "Shaders" (файлы не компилируются VS,
# только отображаются и участвуют в отслеживании изменений через custom command DEPENDS).
if(SHADER_SOURCES)
    target_sources(vulkan_tutorial PRIVATE ${SHADER_SOURCES})
    set_source_files_properties(${SHADER_SOURCES} PROPERTIES
        VS_TOOL_OVERRIDE "None"
        HEADER_FILE_ONLY TRUE
    )
    source_group(TREE "${SHADER_SOURCE_DIR}" PREFIX "Shaders" FILES ${SHADER_SOURCES})
endif()

if(GLSLC_EXECUTABLE)
    set(COMPILED_SHADERS "")

    foreach(SHADER_FILE ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        set(SPIRV_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
            COMMAND ${GLSLC_EXECUTABLE} "${SHADER_FILE}" -o "${SPIRV_OUTPUT}"
            DEPENDS "${SHADER_FILE}"
            COMMENT "Compiling shader: ${SHADER_NAME}"
            VERBATIM
        )
        list(APPEND COMPILED_SHADERS "${SPIRV_OUTPUT}")
    endforeach()

    add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})
    add_dependencies(vulkan_tutorial compile_shaders)

    add_custom_command(TARGET vulkan_tutorial POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADER_BINARY_DIR}"
            "$<TARGET_FILE_DIR:vulkan_tutorial>/Shaders/bin"
        COMMENT "Copying compiled shaders..."
    )
else()
    message(WARNING "glslc not found, shaders will not be compiled")
endif()

set_target_properties(vulkan_tutorial PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(vulkan_tutorial PRIVATE
    glm::glm
    glfw
    tinyobjloader
    ${Vulkan_LIBRARIES}
)

# ---- Windows Release package (opt-in) ----
# The normal Debug build does not enable these install rules.
if(DE_PACKAGE_RELEASE)
    if(NOT WIN32)
        message(FATAL_ERROR "DE_PACKAGE_RELEASE is intended for Windows")
    endif()
    if(NOT GLSLC_EXECUTABLE OR NOT EXISTS "${GLSLC_EXECUTABLE}")
        message(FATAL_ERROR "glslc is required for DE_PACKAGE_RELEASE")
    endif()
    if(NOT TARGET compile_shaders OR NOT COMPILED_SHADERS)
        message(FATAL_ERROR "No compiled shader outputs were configured")
    endif()

    # Runtime DLLs are searched next to the compiler and, if these targets
    # are shared libraries, next to their build outputs as well.
    get_filename_component(DE_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(DE_RUNTIME_SEARCH_DIRS "${DE_COMPILER_BIN}")
    foreach(DE_DEP_TARGET IN ITEMS glfw glm::glm tinyobjloader)
        if(TARGET "${DE_DEP_TARGET}")
            get_target_property(DE_DEP_TYPE "${DE_DEP_TARGET}" TYPE)
            if(DE_DEP_TYPE STREQUAL "SHARED_LIBRARY" OR DE_DEP_TYPE STREQUAL "MODULE_LIBRARY")
                list(APPEND DE_RUNTIME_SEARCH_DIRS "$<TARGET_FILE_DIR:${DE_DEP_TARGET}>")
            endif()
        endif()
    endforeach()

    # Refuse to assemble a Debug or other configuration as a Release package.
    install(CODE [=[
        if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL "Release")
            message(FATAL_ERROR "The DE_Runtime package requires the Release configuration")
        endif()
    ]=] COMPONENT DE_Runtime)

    install(TARGETS vulkan_tutorial
        RUNTIME DESTINATION "." COMPONENT DE_Runtime
    )

    # Scan the executable's imported DLLs recursively. We use file() here
    # rather than an install dependency set so DLLs built by this project are
    # not automatically excluded from the scan.
    set(DE_RUNTIME_DIR_LINES "")
    foreach(DE_RUNTIME_DIR IN LISTS DE_RUNTIME_SEARCH_DIRS)
        string(APPEND DE_RUNTIME_DIR_LINES "\n            \"${DE_RUNTIME_DIR}\"")
    endforeach()
    set(DE_INSTALL_DLL_CODE [=[
        file(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR DE_RESOLVED_DLLS
            EXECUTABLES "$<TARGET_FILE:vulkan_tutorial>"
            DIRECTORIES @DE_RUNTIME_DIR_LINES@
            PRE_EXCLUDE_REGEXES
                "^api-ms-win-.*"
                "^ext-ms-win-.*"
                "^vulkan-1\\.dll$"
            POST_EXCLUDE_REGEXES
                "[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\]"
                "[/\\\\][Ss][Yy][Ss][Ww][Oo][Ww]64[/\\\\]"
        )
        foreach(DE_DLL IN LISTS DE_RESOLVED_DLLS)
            file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/."
                TYPE SHARED_LIBRARY FILES "${DE_DLL}")
        endforeach()
    ]=])
    string(CONFIGURE "${DE_INSTALL_DLL_CODE}" DE_INSTALL_DLL_CODE @ONLY)
    install(CODE "${DE_INSTALL_DLL_CODE}" COMPONENT DE_Runtime)

    # Only outputs from configured shader sources, not arbitrary old .spv files.
    install(FILES ${COMPILED_SHADERS}
        DESTINATION "Shaders/bin" COMPONENT DE_Runtime
    )

    # Copy complete asset directories. Omit generated OBJ caches so that
    # caches created by another build/compiler are not reused accidentally.
    install(DIRECTORY
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_SOURCE_DIR}/Models"
        "${CMAKE_SOURCE_DIR}/Materials"
        "${CMAKE_SOURCE_DIR}/Fonts"
        DESTINATION "." COMPONENT DE_Runtime
        PATTERN "*.obj.mesh" EXCLUDE
        PATTERN "*.obj.matCache" EXCLUDE
    )

    # The application uses relative paths. This launcher makes the package
    # directory the working directory without changing the C++ application.
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/de-package-run.bat"
        CONTENT [=[@echo off
setlocal
cd /d "%~dp0" || exit /b 1
"%~dp0$<TARGET_FILE_NAME:vulkan_tutorial>" %*
exit /b %ERRORLEVEL%
]=]
        NEWLINE_STYLE WIN32
    )
    install(FILES "${CMAKE_BINARY_DIR}/de-package-run.bat"
        DESTINATION "." RENAME "run.bat" COMPONENT DE_Runtime
    )

    set(DE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/release-package" CACHE PATH
        "Output directory for the Release runtime package")

    # Depends on the executable, which itself depends on compile_shaders.
    # Installation overlays the directory; it does not delete old files.
    add_custom_target(stage_release
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>" --component DE_Runtime --prefix "${DE_PACKAGE_DIR}"
        DEPENDS vulkan_tutorial
        COMMENT "Assembling Windows Release package in ${DE_PACKAGE_DIR}"
        USES_TERMINAL
        VERBATIM
    )
endif()
