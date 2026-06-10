cmake_minimum_required(VERSION 3.20) 
cmake_policy(SET CMP0091 NEW)

project("vulkan-cpp-tutorial")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3)
find_package(fmt)
find_program(GLSLC_EXECUTABLE glslc)


include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG       8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # 1.0.3
)

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

add_executable(vulkan_tutorial 
    src/main.cpp
    src/DE_Application.cpp
    src/DE_Device.cpp
    src/DE_Instance.cpp
    src/DE_Memory.cpp
    src/DE_Shaders.cpp
    src/DE_Pipeline.cpp
    src/DE_Descriptors.cpp
    src/DE_CommandBuffers.cpp
    src/DE_Window.cpp
)

# ---- Компиляция шейдеров (после создания цели) ----
if(GLSLC_EXECUTABLE)
    add_custom_target(compile_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/src/Shaders/bin"
        COMMAND ${GLSLC_EXECUTABLE} "${CMAKE_SOURCE_DIR}/src/Shaders/compute.comp" -o "${CMAKE_SOURCE_DIR}/src/Shaders/bin/compute.spv"
        COMMAND ${GLSLC_EXECUTABLE} "${CMAKE_SOURCE_DIR}/src/Shaders/vertex.vert"   -o "${CMAKE_SOURCE_DIR}/src/Shaders/bin/vertex.spv"
        COMMAND ${GLSLC_EXECUTABLE} "${CMAKE_SOURCE_DIR}/src/Shaders/fragment.frag" -o "${CMAKE_SOURCE_DIR}/src/Shaders/bin/fragment.spv"
        COMMENT "Compiling shaders..."
    )
    add_dependencies(vulkan_tutorial compile_shaders)

    add_custom_command(TARGET vulkan_tutorial POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/src/Shaders/bin"
            "$<TARGET_FILE_DIR:vulkan_tutorial>/Shaders/bin"
        COMMENT "Copying compiled shaders..."
    )
else()
    message(WARNING "glslc not found, shaders will not be compiled")
endif()

set_target_properties(vulkan_tutorial PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src"
)

FetchContent_MakeAvailable(glm)

target_link_libraries(vulkan_tutorial PRIVATE 
    fmt::fmt
    glm::glm
    glfw
    ${Vulkan_LIBRARIES}
)

target_include_directories(vulkan_tutorial PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
)