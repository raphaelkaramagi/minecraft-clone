#include "World.h"
#include <iostream> // For debug

World::World() {
    // Constructor - maybe load a few initial chunks?
    // For now, let's create a 3x1x3 area of chunks around origin (0,0,0) for testing.
    // The Y chunk coordinate will be 0 for now (single layer of chunks vertically)
    for (int x = -1; x <= 1; ++x) {
        for (int z = -1; z <= 1; ++z) {
            ensureChunkExists(glm::ivec3(x, 0, z));
        }
    }
}

World::~World() {
    // Destructor - m_chunks with unique_ptr will auto-cleanup
}

bool World::ensureChunkExists(glm::ivec3 chunkCoord) {
    if (m_chunks.find(chunkCoord) == m_chunks.end()) {
        m_chunks[chunkCoord] = std::make_unique<Chunk>(chunkCoord);
        m_chunks[chunkCoord]->generateSimpleTerrain(); // Generate terrain for the new chunk
        // std::cout << "World: Generated chunk at " << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
        return true;
    }
    return false;
}

Chunk* World::getChunk(glm::ivec3 chunkCoord) const {
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end()) {
        return it->second.get(); // Return raw pointer from unique_ptr
    }
    return nullptr;
}

BlockType World::getBlock(glm::ivec3 worldBlockPos) const {
    glm::ivec3 chunkCoord = worldBlockToChunkCoord(worldBlockPos);
    Chunk* chunk = getChunk(chunkCoord);
    if (chunk) {
        glm::ivec3 localPos = worldBlockToLocalCoord(worldBlockPos);
        return chunk->getBlock(localPos.x, localPos.y, localPos.z);
    }
    return BlockType::Air; // Or some other default if chunk doesn't exist
}

void World::setBlock(glm::ivec3 worldBlockPos, BlockType type) {
    glm::ivec3 chunkCoord = worldBlockToChunkCoord(worldBlockPos);
    ensureChunkExists(chunkCoord); // Ensure chunk exists before setting a block
    Chunk* chunk = getChunk(chunkCoord);
    if (chunk) {
        glm::ivec3 localPos = worldBlockToLocalCoord(worldBlockPos);
        chunk->setBlock(localPos.x, localPos.y, localPos.z, type);
    }
}

const std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare>& World::getLoadedChunks() const {
    return m_chunks;
}

// Helper to convert world block coordinates to chunk coordinates
glm::ivec3 World::worldBlockToChunkCoord(glm::ivec3 worldBlockPos) const {
    return glm::ivec3(
        static_cast<int>(floor(static_cast<float>(worldBlockPos.x) / Chunk::CHUNK_WIDTH)),
        static_cast<int>(floor(static_cast<float>(worldBlockPos.y) / Chunk::CHUNK_HEIGHT)),
        static_cast<int>(floor(static_cast<float>(worldBlockPos.z) / Chunk::CHUNK_DEPTH))
    );
}

// Helper to convert world block coordinates to local block coordinates within a chunk
glm::ivec3 World::worldBlockToLocalCoord(glm::ivec3 worldBlockPos) const {
    return glm::ivec3(
        (worldBlockPos.x % Chunk::CHUNK_WIDTH + Chunk::CHUNK_WIDTH) % Chunk::CHUNK_WIDTH, // Positive modulo
        (worldBlockPos.y % Chunk::CHUNK_HEIGHT + Chunk::CHUNK_HEIGHT) % Chunk::CHUNK_HEIGHT,
        (worldBlockPos.z % Chunk::CHUNK_DEPTH + Chunk::CHUNK_DEPTH) % Chunk::CHUNK_DEPTH
    );
} 