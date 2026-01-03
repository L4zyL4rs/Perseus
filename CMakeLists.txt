cmake_minimum_required(VERSION 3.15)
project(Perseus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ECS library
add_library(ecs INTERFACE)
target_include_directories(ecs INTERFACE include/ecs)

add_library(systems INTERFACE)
target_include_directories(systems INTERFACE include/systems)
target_link_libraries(systems INTERFACE ecs)

add_library(scheduler INTERFACE)
target_include_directories(scheduler INTERFACE include/scheduler)
target_link_libraries(scheduler INTERFACE systems ecs)

add_library(config INTERFACE)
target_include_directories(config INTERFACE include/config)

# Sandbox app
add_executable(sandbox "apps/sandbox/templating.cpp" "include/ecs/EntityBuilder.h"  "include/ecs/ECSView.h" "include/ecs/Meta.h" "include/systems/systemBase.h" "include/config/config.h" "apps/sandbox/main.cpp" "include/systems/testSystem1.h" "include/systems/testSystem2.h")
target_link_libraries(sandbox PRIVATE ecs scheduler systems config)

# testECS app
add_executable(testECS "tests/ecs/testECS.cpp" "include/ecs/EntityBuilder.h"  "include/ecs/Registry.h" "include/systems/systemBase.h" "include/config/config.h" "include/systems/testSystem1.h" "include/systems/testSystem2.h")
target_link_libraries(testECS PRIVATE ecs scheduler systems config)