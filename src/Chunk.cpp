#include "Chunk.h"
#include <iostream> // For debug output
#include <glad/glad.h> // For OpenGL functions
#include <vector> // For std::vector
#include <glm/glm.hpp> // For glm::vec3

// Each vertex: X, Y, Z, R, G, B (0.5f, 0.5f, 0.5f for grey)
// Vertices for a single cube, face by face.
// Each face has 6 vertices (2 triangles), each vertex has 6 floats. Total 36 floats per face.

// Define colors for block types
const glm::vec3 colorStone(0.5f, 0.5f, 0.5f);    // Grey
const glm::vec3 colorDirt(0.6f, 0.4f, 0.2f);     // Brown
const glm::vec3 colorGrassTop(0.0f, 0.8f, 0.0f); // Green for top
const glm::vec3 colorGrassSide(0.5f, 0.35f, 0.15f); // Brownish for sides (used for sides and bottom of grass)
const glm::vec3 colorGrassBottom(0.6f, 0.4f, 0.2f); // Dirt color for grass bottom

// +X (Right)
const float rightFaceVertices[] = {
    0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f, -0.5f,
};
// -X (Left)
const float leftFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
};
// +Y (Top)
const float topFaceVertices[] = {
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
};
// -Y (Bottom)
const float bottomFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
};
// +Z (Front)
const float frontFaceVertices[] = {
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
};
// -Z (Back)
const float backFaceVertices[] = {
     0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
};

const int verticesPerFace = 6; 
const int floatsPerVertexPositionData = 3; // X, Y, Z for the static face data
const int floatsPerVertexRender = 6;       // X, Y, Z, R, G, B for the VBO and rendering
// const int floatsPerFaceData = verticesPerFace * floatsPerVertexPositionData; // 18 floats
const int floatsPerFaceMesh = verticesPerFace * floatsPerVertexRender; // 36 floats (for reservation)

Chunk::Chunk(glm::ivec3 position) 
    : worldPosition(position), m_vao(0), m_vbo(0), m_vertexCount(0), 
      m_isGenerated(false), m_needsMeshBuild(false) { // Initialize new flags
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
                BlockType currentType = BlockType::Air; // Determine block type before calling setBlock directly
                if (y < terrainHeight -1) {
                    currentType = BlockType::Stone;
                } else if (y < terrainHeight) {
                    currentType = BlockType::Dirt;
                } else if (y == terrainHeight) {
                    currentType = BlockType::Grass;
                }
                // Directly set block in m_blocks without triggering a mesh build here
                if (isPositionInBounds(x,y,z)) {
                     m_blocks[coordsToIndex(x,y,z)] = currentType;
                }
            }
        }
    }
    m_isGenerated = true;
    m_needsMeshBuild = true; // Mark for mesh build after terrain is set
    // std::cout << "Chunk generated simple terrain at " << worldPosition.x << ", " << worldPosition.z << std::endl;
    // buildMesh(); // Mesh will be built by World::processWorldUpdates or explicitly
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
    if (m_blocks[coordsToIndex(x,y,z)] != type) { 
        m_blocks[coordsToIndex(x,y,z)] = type;
        m_needsMeshBuild = true; // Mark for rebuild, don't call buildMesh() directly
    }
}

bool Chunk::isPositionInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_WIDTH &&
           y >= 0 && y < CHUNK_HEIGHT &&
           z >= 0 && z < CHUNK_DEPTH;
}

// Helper function to add face vertices to the mesh
void addFace(std::vector<float>& meshVertices, const float* faceVertexPositions, int blockX, int blockY, int blockZ, const glm::vec3& color) {
    for (int i = 0; i < verticesPerFace; ++i) {
        // Position data from faceVertexPositions
        meshVertices.push_back(faceVertexPositions[i * floatsPerVertexPositionData + 0] + blockX); // Vertex X
        meshVertices.push_back(faceVertexPositions[i * floatsPerVertexPositionData + 1] + blockY); // Vertex Y
        meshVertices.push_back(faceVertexPositions[i * floatsPerVertexPositionData + 2] + blockZ); // Vertex Z
        // Color data
        meshVertices.push_back(color.r);
        meshVertices.push_back(color.g);
        meshVertices.push_back(color.b);
    }
}

void Chunk::buildMesh() {
    std::vector<float> localMeshVertices;
    // Estimate a reasonable starting capacity.
    localMeshVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH * floatsPerFaceMesh / 4); 

    // 1. Delete old VAO/VBO if they exist.
    if (m_vao != 0) {
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
        m_vbo = 0;
        m_vao = 0;
    }
    // 2. Always reset vertex count.
    m_vertexCount = 0;

    // 3. Build localMeshVertices 
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                BlockType currentBlockType = getBlock(x, y, z);
                if (currentBlockType != BlockType::Air) {
                    glm::vec3 baseColor; 
                    glm::vec3 faceTopColor = colorGrassTop; 
                    glm::vec3 faceSideColor = colorGrassSide;
                    glm::vec3 faceBottomColor = colorGrassBottom;

                    switch (currentBlockType) {
                        case BlockType::Stone: baseColor = colorStone; break;
                        case BlockType::Dirt: baseColor = colorDirt; break;
                        case BlockType::Grass: /* handled per-face */ break;
                        default: baseColor = glm::vec3(1.0f, 0.0f, 1.0f); break; // Magenta for error
                    }

                    // Check faces and add to localMeshVertices if exposed
                    if (getBlock(x + 1, y, z) == BlockType::Air) { addFace(localMeshVertices, rightFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceSideColor : baseColor); }
                    if (getBlock(x - 1, y, z) == BlockType::Air) { addFace(localMeshVertices, leftFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceSideColor : baseColor); }
                    if (getBlock(x, y + 1, z) == BlockType::Air) { addFace(localMeshVertices, topFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceTopColor : baseColor); }
                    if (getBlock(x, y - 1, z) == BlockType::Air) { addFace(localMeshVertices, bottomFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceBottomColor : baseColor); }
                    if (getBlock(x, y, z + 1) == BlockType::Air) { addFace(localMeshVertices, frontFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceSideColor : baseColor); }
                    if (getBlock(x, y, z - 1) == BlockType::Air) { addFace(localMeshVertices, backFaceVertices, x, y, z, (currentBlockType == BlockType::Grass) ? faceSideColor : baseColor); }
                }
            }
        }
    }

    // 4. If new mesh data exists, create and populate VAO/VBO.
    if (!localMeshVertices.empty()) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, localMeshVertices.size() * sizeof(float), localMeshVertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatsPerVertexRender * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floatsPerVertexRender * sizeof(float), (void*)(floatsPerVertexPositionData * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_vertexCount = static_cast<int>(localMeshVertices.size() / floatsPerVertexRender);
    }
    // else: m_vertexCount remains 0, m_vao remains 0. Nothing to render for this chunk.
    
    m_needsMeshBuild = false;
} 