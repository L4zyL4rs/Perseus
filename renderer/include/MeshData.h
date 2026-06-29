#pragma once
#include <vector>
#include "Vertex.h"

// Actual vertices for loading and manipulation
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

