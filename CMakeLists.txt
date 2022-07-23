cmake_minimum_required(VERSION 3.22)
project(Vulkan C)

set(CMAKE_C_STANDARD 23)

add_executable(Vulkan src/Main.c)
target_compile_options(Vulkan PRIVATE -W4 -Werror)
target_include_directories(Vulkan PRIVATE $ENV{VULKAN_SDK}/Include)
target_link_directories(Vulkan PRIVATE $ENV{VULKAN_SDK}/Lib)
target_link_libraries(Vulkan PRIVATE vulkan-1)
