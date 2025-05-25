#include "Chunk.h"
#include <iostream> // For debug output
#include <glad/glad.h> // For OpenGL functions
#include <vector> // For std::vector

// Each vertex: X, Y, Z, R, G, B (0.5f, 0.5f, 0.5f for grey)
// Vertices for a single cube, face by face.
// Each face has 6 vertices (2 triangles), each vertex has 6 floats. Total 36 floats per face.

// +X (Right)
const float rightFaceVertices[] = {
    0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
};
// -X (Left)
const float leftFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
};
// +Y (Top)
const float topFaceVertices[] = {
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
};
// -Y (Bottom)
const float bottomFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
};
// +Z (Front)
const float frontFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
};
// -Z (Back)
const float backFaceVertices[] = {
     0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
};

const int verticesPerFace = 6; 
const int floatsPerVertex = 6; // 3 for pos, 3 for color
const int floatsPerFace = verticesPerFace * floatsPerVertex; // 36

Chunk::Chunk(glm::ivec3 position) : worldPosition(position), m_vao(0), m_vbo(0), m_vertexCount(0) {
    m_blocks.resize(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, BlockType::Air);
    // std::cout << "Chunk created at: " << position.x << ", " << position.y << ", " << position.z << std::endl;
}

Chunk::~Chunk() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    // std::cout << "Chunk destroyed: " << worldPosition.x << ", " << worldPosition.y << ", " << worldPosition.z << std::endl;
}

// Helper to convert 3D local coords to 1D array index
int Chunk::coordsToIndex(int x, int y, int z) const {
    // Assumes x, y, z are already validated by isPositionInBounds if necessary
    return x + y * CHUNK_WIDTH + z * CHUNK_WIDTH * CHUNK_HEIGHT;
}

void Chunk::generateSimpleTerrain() {
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            int terrainHeight = CHUNK_HEIGHT / 2; // Example:
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                if (y < terrainHeight -1) {
                    setBlock(x, y, z, BlockType::Stone);
                } else if (y < terrainHeight) {
                    setBlock(x, y, z, BlockType::Dirt);
                } else if (y == terrainHeight) {
                    setBlock(x, y, z, BlockType::Grass);
                } else {
                    setBlock(x, y, z, BlockType::Air);
                }
            }
        }
    }
    // std::cout << "Chunk generated simple terrain at " << worldPosition.x << ", " << worldPosition.z << std::endl;
    buildMesh(); // Build mesh after terrain is generated
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!isPositionInBounds(x, y, z)) {
        return BlockType::Air;
    }
    return m_blocks[coordsToIndex(x,y,z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (!isPositionInBounds(x, y, z)) {
        return;
    }
    m_blocks[coordsToIndex(x,y,z)] = type;
    // Future: Mark chunk as dirty and needing a mesh rebuild
    // buildMesh(); // For now, rebuild mesh on any change (can be inefficient)
}

bool Chunk::isPositionInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_WIDTH &&
           y >= 0 && y < CHUNK_HEIGHT &&
           z >= 0 && z < CHUNK_DEPTH;
}

// Helper function to add face vertices to the mesh
void addFace(std::vector<float>& meshVertices, const float* faceData, int blockX, int blockY, int blockZ) {
    for (int i = 0; i < verticesPerFace; ++i) {
        meshVertices.push_back(faceData[i * floatsPerVertex + 0] + blockX);
        meshVertices.push_back(faceData[i * floatsPerVertex + 1] + blockY);
        meshVertices.push_back(faceData[i * floatsPerVertex + 2] + blockZ);
        meshVertices.push_back(faceData[i * floatsPerVertex + 3]);
        meshVertices.push_back(faceData[i * floatsPerVertex + 4]);
        meshVertices.push_back(faceData[i * floatsPerVertex + 5]);
    }
}

void Chunk::buildMesh() {
    std::vector<float> localMeshVertices;
    // Estimate: worst case is a checkerboard, so roughly half blocks * 6 faces. More realistically far less.
    localMeshVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH * floatsPerFace * 3); 

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                BlockType currentBlockType = getBlock(x, y, z);
                if (currentBlockType != BlockType::Air) {
                    // Check +X face (Right)
                    if (getBlock(x + 1, y, z) == BlockType::Air) {
                        addFace(localMeshVertices, rightFaceVertices, x, y, z);
                    }
                    // Check -X face (Left)
                    if (getBlock(x - 1, y, z) == BlockType::Air) {
                        addFace(localMeshVertices, leftFaceVertices, x, y, z);
                    }
                    // Check +Y face (Top)
                    if (getBlock(x, y + 1, z) == BlockType::Air) {
                        addFace(localMeshVertices, topFaceVertices, x, y, z);
                    }
                    // Check -Y face (Bottom)
                    if (getBlock(x, y - 1, z) == BlockType::Air) {
                        addFace(localMeshVertices, bottomFaceVertices, x, y, z);
                    }
                    // Check +Z face (Front)
                    if (getBlock(x, y, z + 1) == BlockType::Air) {
                        addFace(localMeshVertices, frontFaceVertices, x, y, z);
                    }
                    // Check -Z face (Back)
                    if (getBlock(x, y, z - 1) == BlockType::Air) {
                        addFace(localMeshVertices, backFaceVertices, x, y, z);
                    }
                }
            }
        }
    }

    if (m_vao != 0) {
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
        m_vbo = 0;
        m_vao = 0;
    }
    m_vertexCount = 0;

    if (!localMeshVertices.empty()) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, localMeshVertices.size() * sizeof(float), localMeshVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_vertexCount = static_cast<int>(localMeshVertices.size() / floatsPerVertex);
        // std::cout << "Chunk at " << worldPosition.x << "," << worldPosition.y << "," << worldPosition.z << " built mesh with " << m_vertexCount << " vertices." << std::endl;
    } else {
        // std::cout << "Chunk at " << worldPosition.x << "," << worldPosition.y << "," << worldPosition.z << " built empty mesh." << std::endl;
    }
} 