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

target_include_directories(vulkan_test PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_test PUBLIC cxx_std glfw3 vulkan-1)