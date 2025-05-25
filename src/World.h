#ifndef WORLD_H
#define WORLD_H

#include "Chunk.h"
#include "BlockType.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For ivec3 comparison if needed, though not directly
#include <map>
#include <memory> // For std::unique_ptr

// Custom comparator for glm::ivec3 to use it as a key in std::map
struct Ivec3Compare {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

class World {
public:
    World();
    ~World();

    // Generates or loads a chunk at the given chunk coordinates
    // Returns true if a new chunk was generated/loaded, false if it already existed or failed
    bool ensureChunkExists(glm::ivec3 chunkCoord);
    Chunk* getChunk(glm::ivec3 chunkCoord) const; // Get a non-owning pointer to a chunk

    BlockType getBlock(glm::ivec3 worldBlockPos) const;
    void setBlock(glm::ivec3 worldBlockPos, BlockType type);

    // For iteration by the renderer (temporary)
    const std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare>& getLoadedChunks() const;

private:
    std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare> m_chunks;

    glm::ivec3 worldBlockToChunkCoord(glm::ivec3 worldBlockPos) const;
    glm::ivec3 worldBlockToLocalCoord(glm::ivec3 worldBlockPos) const;
};

#endif // WORLD_H 