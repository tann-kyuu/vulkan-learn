cmake_minimum_required(VERSION 3.30)
project(j_test LANGUAGES CXX VERSION 1.0.0)

# C++20 Standard Lib
add_library(cxx_std INTERFACE)
target_compile_features(cxx_std INTERFACE cxx_std_20)

# Executable from sources
add_executable(vulkan_test test.cpp test_vulkan.cpp)
target_link_libraries(vulkan_test PRIVATE cxx_std)

# Import glfw from local direction
target_include_directories(vulkan_test PUBLIC "glfw/include")
target_link_directories(vulkan_test PUBLIC "glfw/lib-mingw-w64")
target_link_libraries(vulkan_test PRIVATE glfw3)

# Import Vulkan from system
find_package(Vulkan REQUIRED)
target_link_libraries(vulkan_test PRIVATE Vulkan::Vulkan)

# Find glslangValidator
find_program(GLSLANG_VALIDATOR glslangValidator)

# List of shader source files
set(SHADER_FILES
    shaders/shader.vert
    shaders/shader.frag
)

# Generate SPIR-V binary files from each GLSL shaders
foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(SPIRV_OUTPUT ${CMAKE_BINARY_DIR}/${FILENAME}.spv)

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_SOURCE_DIR}/${SHADER} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER} to SPIR-V"
    )
    list(APPEND SPIRV_FILES ${SPIRV_OUTPUT})
endforeach()

# Add custom target to build all SPIR-V files
add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})


