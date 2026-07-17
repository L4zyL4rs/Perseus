cmake_minimum_required(VERSION 3.15)
project(Perseus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(renderer)

file(COPY
    ${CMAKE_SOURCE_DIR}/assets/models
    DESTINATION ${CMAKE_BINARY_DIR}
)

file(COPY
    ${CMAKE_SOURCE_DIR}/assets/textures
    DESTINATION ${CMAKE_BINARY_DIR}
)

file(MAKE_DIRECTORY ${FONT_DIR})
set(FONT_DIR ${CMAKE_BINARY_DIR}/assets/fonts)

file(DOWNLOAD 
https://github.com/google/fonts/raw/892dc85555606db5a08545c7874a4dc561d3ce50/apache/robotoslab/RobotoSlab%5Bwght%5D.ttf
${FONT_DIR}/robotoslab.ttf)

# Shader compilation
find_program(GLSLC glslc REQUIRED)

  set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
  set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/assets/shaders)

  file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
      "${SHADER_SOURCE_DIR}/*.vert"
      "${SHADER_SOURCE_DIR}/*.frag"
      "${SHADER_SOURCE_DIR}/*.geom"
  )

  set(SHADER_OUTPUTS)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)

    if(SHADER_EXT STREQUAL ".vert")
        set(STAGE_SUFFIX "Vert")
    elseif(SHADER_EXT STREQUAL ".frag")
        set(STAGE_SUFFIX "Frag")
    elseif(SHADER_EXT STREQUAL ".geom")
        set(STAGE_SUFFIX "Geom")
    endif()

    set(SPV "${SHADER_BINARY_DIR}/${SHADER_NAME}${STAGE_SUFFIX}.spv")

    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
        VERBATIM
    )

    list(APPEND SHADER_OUTPUTS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})

# ECS library
add_library(ecs INTERFACE)
target_include_directories(ecs INTERFACE include/ecs)

add_library(components INTERFACE)
target_include_directories(components INTERFACE include/components)

add_library(systems INTERFACE)
target_include_directories(systems INTERFACE include/systems)
target_link_libraries(systems INTERFACE ecs VulkanRenderer components)

add_library(scheduler INTERFACE)
target_include_directories(scheduler INTERFACE include/scheduler)
target_link_libraries(scheduler INTERFACE systems ecs)

add_library(config INTERFACE)
target_include_directories(config INTERFACE include/config)


# Sandbox app
add_executable(sandbox "apps/sandbox/templating.cpp" "include/ecs/EntityBuilder.h"  "include/ecs/ECSView.h" "include/ecs/Meta.h" "include/systems/systemBase.h" "include/config/ECSConfig.h" "apps/sandbox/main.cpp" "include/systems/testSystem1.h" "include/systems/testSystem2.h")
target_link_libraries(sandbox PRIVATE ecs scheduler systems config)
add_dependencies(sandbox Shaders)

# testECS app
add_executable(testECS "tests/ecs/testECS.cpp" "include/ecs/EntityBuilder.h"  "include/ecs/Registry.h" "include/systems/systemBase.h" "include/config/ECSConfig.h" "include/systems/testSystem1.h" "include/systems/testSystem2.h")
target_link_libraries(testECS PRIVATE ecs scheduler systems config)
