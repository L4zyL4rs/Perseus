#pragma once
#include "MeshData.h"
#include <ios>
#include <stdexcept>

enum class GenerateMeshType {
    flat
};

// This can be extended in the future to allow for different struct implementations or different types
struct GenerateMeshInfo {
    GenerateMeshType type;
    uint32_t xSize;
    uint32_t ySize;
    float scale;
    glm::vec3 color;
};

namespace MeshGenerator {
inline MeshData generate(GenerateMeshInfo info) {
    if(info.ySize == 0 || info.xSize == 0) {
        throw std::domain_error("Mesh must at least be 1x1 in size");
    }
    // Generate grid of vertices, then figure out how to index them
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t currentIndex = 0;

    int downIndex = - (info.xSize + 1);
    int leftIndex = - 1;
    int downLeftIndex = - (info.xSize + 1) - 1;

    indices.reserve(info.xSize * info.ySize * 6);
    Vertex vertex;
    vertex.pos = glm::vec3(0,0,0);
    vertex.color = info.color;
    vertex.texCoord = glm::vec3(0);
    for(int y = 0; y <= info.ySize; y++) {
        for(int x = 0; x <= info.xSize; x++) {
            vertex.pos.x = info.scale * x;
            vertex.pos.y = info.scale * y;
            vertices.emplace_back(vertex);
            if(x > 0 && y > 0) {
                indices.push_back(currentIndex + downIndex);
                indices.push_back(currentIndex + leftIndex);
                indices.push_back(currentIndex + downLeftIndex);
                indices.push_back(currentIndex + downIndex);
                indices.push_back(currentIndex);
                indices.push_back(currentIndex + leftIndex);
            }
            currentIndex++;
        }
    }

    std::cout << "Size of vertices: " << vertices.size() << "\n";
    MeshData mesh;
    mesh.vertices = vertices;
    mesh.indices = indices;

    return mesh;
}
} // Namespace

