# Perseus
Small experimental ECS-based game engine using Vulkan, for experimentation/learning purposes.

## Requirements
- C++23 or higher
- CMake >= 3.20
- Vulkan compatible GPU
- Vulkan Drivers and SDK including validation layers
- (OpenGL SDK is hopefully not needed anymore)

### Vulkan Instructions
- Windows: Download  Vulkan SDK from https://vulkan.lunarg.com/sdk/home and follow the instructions
- Linux: Follow installation instructions on https://vulkan.lunarg.com/doc/sdk/1.4.341.1/linux/getting_started.html

## Installation
    git clone https://github.com/L4zyL4rs/Perseus.git
    cd Perseus
    cmake -S . -B out
    cmake --build out

Remaining external dependencies will be fetched by CMake.
