#ifndef CHUNK_H
#define CHUNK_H

#include "BlockType.h"
#include <vector>
#include <glm/glm.hpp> // For chunk position (ivec3)
#include <glad/glad.h> // For GLuint
#include <glm/gtc/type_ptr.hpp>

class Chunk {
public:
    static const int CHUNK_WIDTH = 16;  // X dimension
    static const int CHUNK_HEIGHT = 16; // Y dimension (Minecraft is 256, but start smaller)
    static const int CHUNK_DEPTH = 16;  // Z dimension

    // Chunk coordinates in the world (not block coordinates)
    glm::ivec3 worldPosition;

    Chunk(glm::ivec3 position);
    ~Chunk();

    void generateSimpleTerrain(); // Fills the chunk with some basic terrain

    BlockType getBlock(int x, int y, int z) const; // Local coordinates within the chunk
    void setBlock(int x, int y, int z, BlockType type); // Local coordinates

    bool isPositionInBounds(int x, int y, int z) const;

    void buildMesh(); // Generates the VAO/VBO for this chunk's visible faces
    
    // Getter for renderer
    GLuint getVAO() const { return m_vao; }
    int getVertexCount() const { return m_vertexCount; }
    bool hasMesh() const { return m_vao != 0 && m_vertexCount > 0; }

    glm::ivec3 getWorldPosition() const { return worldPosition; }

    // New flags for deferred processing
    bool isGenerated() const { return m_isGenerated; }
    void setGenerated(bool generated) { m_isGenerated = generated; }
    bool needsMeshBuild() const { return m_needsMeshBuild; }
    void setNeedsMeshBuild(bool needsBuild) { m_needsMeshBuild = needsBuild; }

    // For later: methods to build a mesh from the chunk data
    // void buildMesh();
    // const std::vector<float>& getMeshVertices() const;
    // const std::vector<unsigned int>& getMeshIndices() const;

private:
    // 3D array of blocks. std::vector might be slow for direct access.
    // A flat 1D array is usually more cache-friendly and faster.
    // Access via: blocks[x + y * CHUNK_WIDTH + z * CHUNK_WIDTH * CHUNK_HEIGHT]
    std::vector<BlockType> m_blocks;

    GLuint m_vao;
    GLuint m_vbo;
    // GLuint m_ebo; // If using indexed drawing later
    int m_vertexCount;
    // std::vector<float> m_meshVertices; // Temporary storage during buildMesh, not kept as member

    bool m_isGenerated;      // True if generateSimpleTerrain has run
    bool m_needsMeshBuild;   // True if blocks changed and mesh needs rebuild

    // Helper to convert 3D local coords to 1D array index
    int coordsToIndex(int x, int y, int z) const;
};

#endif // CHUNK_H 