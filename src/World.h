#ifndef WORLD_H
#define WORLD_H

#include "Chunk.h"
#include "BlockType.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For ivec3 comparison if needed, though not directly
#include <glm/gtx/hash.hpp>
#include <vector> // For storing collision AABBs
#include <map>
#include <memory> // For std::unique_ptr

// Forward declare AABB from Camera.h or define it here if preferred (Camera.h is fine)
struct AABB; // Assuming AABB is defined in Camera.h and Camera.h will be included where World is used

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
    // New struct for raycasting results
    struct RaycastResult {
        bool hit = false;
        glm::ivec3 blockHit;      // The solid block that was hit
        glm::ivec3 blockBefore;   // The air block position just before hitting blockHit
    };

    World();
    ~World();
    void init(); // New method to initialize world (e.g., create initial chunks)

    // Generates or loads a chunk at the given chunk coordinates
    // Returns true if a new chunk was generated/loaded, false if it already existed or failed
    bool ensureChunkExists(glm::ivec3 chunkCoord);
    Chunk* getChunk(glm::ivec3 chunkCoord) const; // Get a non-owning pointer to a chunk

    BlockType getBlock(glm::ivec3 worldBlockPos) const;
    void setBlock(glm::ivec3 worldBlockPos, BlockType type);

    // For iteration by the renderer (temporary)
    const std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare>& getLoadedChunks() const;

    // New raycasting method
    RaycastResult castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float maxDistance) const;

    void processWorldUpdates(); // New method for deferred chunk processing

    // Collision detection
    // Checks collision for the playerAABB, attempts to resolve it by adjusting playerAABB and velocity.
    // Returns true if any collision occurred and was resolved.
    bool resolveCollisions(AABB& playerAABB, glm::vec3& playerVelocity, bool& io_isOnGround);

    // Helper functions - ensuring these are public
    glm::ivec3 worldBlockToChunkCoord(glm::ivec3 worldBlockPos) const;
    glm::ivec3 worldBlockToLocalCoord(glm::ivec3 worldBlockPos) const;

private:
    std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare> m_chunks;
};

#endif // WORLD_H 