cmake_minimum_required(VERSION 3.20)
project(VulkanRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

file(DOWNLOAD 
https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/c5b0ce68f5449667f6efe03cdf2867e2c1e4feb7/include/vk_mem_alloc.h 
${CMAKE_CURRENT_SOURCE_DIR}/external/vk_mem_alloc.h)

file(DOWNLOAD
https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/refs/heads/release/tiny_obj_loader.h
${CMAKE_CURRENT_SOURCE_DIR}/external/tiny_obj_loader.h)

file(DOWNLOAD
https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h
${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image.h)

file(COPY
    ${CMAKE_SOURCE_DIR}/assets
    DESTINATION ${CMAKE_BINARY_DIR}
)

file(MAKE_DIRECTORY ${FONT_DIR})
set(FONT_DIR ${CMAKE_BINARY_DIR}/assets/fonts)

file(DOWNLOAD 
https://github.com/google/fonts/raw/892dc85555606db5a08545c7874a4dc561d3ce50/apache/robotoslab/RobotoSlab%5Bwght%5D.ttf
${FONT_DIR}/robotoslab.ttf)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
include(FetchContent)


FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-14-1
)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glfw freetype glm)

add_library(VulkanRenderer
    source/main.cpp
    source/DescriptorAllocator.cpp
    source/FontRenderer.cpp
    source/PipelineManager.cpp
    source/Quickmath.cpp
    source/renderObject.cpp
    source/VulkanHelper.cpp
    source/vma.cpp  
    source/Config.cpp 
    source/FrameManager.cpp
    source/RenderContext.cpp
    source/Swapchain.cpp
    source/AppWindow.cpp
    source/AssetManager.cpp
    source/int64vector.cpp
    source/ImageManager.cpp
    source/PipelineBuilder.cpp)

target_include_directories(VulkanRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_link_libraries(VulkanRenderer PUBLIC glfw freetype Vulkan::Vulkan glm::glm)
