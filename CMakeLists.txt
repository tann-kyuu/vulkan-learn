cmake_minimum_required(VERSION 3.30)
project(j_test LANGUAGES CXX VERSION 1.0.0)

#C++20 Standard Lib
add_library(cxx_std INTERFACE)
target_compile_features(cxx_std INTERFACE cxx_std_20)

#executable from sources
add_executable(vulkan_test test.cpp test_vulkan.cpp)

#import glfw from local direction
target_include_directories(vulkan_test PUBLIC "glfw/include")
target_link_directories(vulkan_test PUBLIC "glfw/lib")

#import Vulkan from system
find_package(Vulkan REQUIRED)

# Compile shaders using glslc
get_filename_component(GLSLC_COMPILER ${Vulkan_GLSLC_EXECUTABLE} ABSOLUTE)
function(compile_shader SHADER_FILE OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SPV_FILE ${OUTPUT_DIR}/${SHADER_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC_COMPILER} 
            ${SHADER_FILE}
            -o ${SPV_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_FILE} to ${SPV_FILE}"
    )
endfunction()

set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_SPV_DIR "${CMAKE_BINARY_DIR}/shaders")

file(GLOB SHADER_FILES "${SHADER_SRC_DIR}/*")

foreach(SHADER_FILE ${SHADER_FILES})
    compile_shader(${SHADER_FILE} ${SHADER_SPV_DIR})
endforeach()

target_include_directories(vulkan_test PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_test PUBLIC cxx_std glfw3 vulkan-1)