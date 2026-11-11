cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0091 NEW)   # select the MSVC runtime through CMake properties

project(vulkan-cpp-tutorial LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(DE_PACKAGE_RELEASE  "Assemble a self-contained Windows runtime package" OFF)
option(DE_WARNINGS         "Enable -Wall -Wextra for project sources only"     ON)
option(VK_HARD_VALIDATION  "Enable sync and GPU-assisted validation"           OFF)
option(VKENGINE_TIME_TRACE "Emit clang -ftime-trace JSON next to objects"      OFF)

# ---------------------------------------------------------------------------
# Third-party dependencies (FetchContent, no conan)
# SYSTEM keeps their headers out of our warning stream.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        8d1fd52e5ab5590e2c81768ace50c72bae28f2ed  # 1.0.3
    SYSTEM
)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579  # 3.4
    SYSTEM
)

# GLFW: the library only, no examples, tests, docs or install rules.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        45636bdcef1a4fec140346b90c0b50bf0bc3e23b  # 2026-06-19
    SYSTEM
)

FetchContent_MakeAvailable(glm glfw tinyobjloader)

# ---------------------------------------------------------------------------
# Vulkan SDK and shader compiler
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)   # honours the VULKAN_SDK environment variable

find_program(GLSLC_EXECUTABLE glslc
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")

# ---------------------------------------------------------------------------
# Sources
# Every .cpp/.hpp under src/ is picked up automatically; third-party code is
# collected separately so it never receives our warning flags.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE DE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE DE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.h")

list(FILTER DE_SOURCES EXCLUDE REGEX "/third_party/")
list(FILTER DE_HEADERS EXCLUDE REGEX "/third_party/")

file(GLOB_RECURSE DE_THIRD_PARTY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/third_party/*.cpp")

# Vendored third-party libraries (imgui and friends) build as a plain static
# library: no project warnings, headers exposed as SYSTEM.
add_library(de_third_party STATIC ${DE_THIRD_PARTY_SOURCES})
target_include_directories(de_third_party SYSTEM PUBLIC
    "${CMAKE_SOURCE_DIR}/src/third_party"
    "${CMAKE_SOURCE_DIR}/src/third_party/imgui"
    "${CMAKE_SOURCE_DIR}/src/third_party/imgui/backends")

target_link_libraries(de_third_party PRIVATE Vulkan::Vulkan)
# ImGui backends are compiled here, so GLFW and Vulkan must be visible to them.
target_link_libraries(de_third_party PUBLIC glfw Vulkan::Vulkan)
# Headers are listed in the target so an IDE shows them; they are never
# compiled on their own.

add_executable(vulkan_tutorial ${DE_SOURCES} ${DE_HEADERS})

target_include_directories(vulkan_tutorial PRIVATE "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(vulkan_tutorial PRIVATE
    de_third_party
    glm::glm
    glfw
    tinyobjloader
    Vulkan::Vulkan
)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
if(DE_WARNINGS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(vulkan_tutorial PRIVATE -Wall -Wextra)
endif()

if(VKENGINE_TIME_TRACE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(vulkan_tutorial PRIVATE
        -ftime-trace
        -ftime-trace-granularity=500)
endif()

if(VK_HARD_VALIDATION)
    target_compile_definitions(vulkan_tutorial PRIVATE DE_VK_HARD_VALIDATION=1)
endif()

# Run the executable from the project root when started from an IDE, because
# the application loads assets through relative paths.
set_target_properties(vulkan_tutorial PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Shaders
# Every file with a shader extension is compiled, so adding a shader needs no
# CMake change. Output goes to Shaders/bin, the path the application reads.
# ---------------------------------------------------------------------------
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Shaders")

if(DE_PACKAGE_RELEASE)
    # Packaged builds write SPIR-V outside the source tree.
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

    # Keep a copy next to the executable as well.
    add_custom_command(TARGET vulkan_tutorial POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADER_BINARY_DIR}"
            "$<TARGET_FILE_DIR:vulkan_tutorial>/Shaders/bin"
        COMMENT "Copying compiled shaders..."
    )
else()
    message(WARNING "glslc not found, shaders will not be compiled")
endif()

# ---------------------------------------------------------------------------
# Windows runtime package (opt-in)
# ---------------------------------------------------------------------------
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

    # Refuse to package anything but an actual Release build.
    install(CODE [=[
        if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL "Release")
            message(FATAL_ERROR "The DE_Runtime package requires the Release configuration")
        endif()
    ]=] COMPONENT DE_Runtime)

    install(TARGETS vulkan_tutorial
        RUNTIME DESTINATION "." COMPONENT DE_Runtime
    )

    # Runtime DLLs are looked up next to the compiler and next to the build
    # outputs of any dependency that is built as a shared library.
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

    # Scan the executable's imported DLLs recursively. file() is used instead of
    # an install dependency set so DLLs built by this project are not skipped.
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

    # Only configured shader outputs, never stale .spv files.
    install(FILES ${COMPILED_SHADERS}
        DESTINATION "Shaders/bin" COMPONENT DE_Runtime
    )

    # Asset directories, without generated model caches from other builds.
    set(DE_ASSET_DIRS
        "${CMAKE_SOURCE_DIR}/Models"
        "${CMAKE_SOURCE_DIR}/Materials"
        "${CMAKE_SOURCE_DIR}/Fonts"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
        list(APPEND DE_ASSET_DIRS "${CMAKE_SOURCE_DIR}/assets")
    endif()

    install(DIRECTORY ${DE_ASSET_DIRS}
        DESTINATION "." COMPONENT DE_Runtime
        PATTERN "*.obj.mesh" EXCLUDE
        PATTERN "*.obj.matCache" EXCLUDE
    )

    # The application uses relative paths, so the launcher makes the package
    # directory the working directory.
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

    # Depends on the executable, which depends on compile_shaders. Installation
    # overlays the directory and does not delete stale files.
    add_custom_target(stage_release
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>" --component DE_Runtime --prefix "${DE_PACKAGE_DIR}"
        DEPENDS vulkan_tutorial
        COMMENT "Assembling Windows Release package in ${DE_PACKAGE_DIR}"
        USES_TERMINAL
        VERBATIM
    )
endif()