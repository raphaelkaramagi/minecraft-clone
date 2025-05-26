#include "World.h"
#include "Camera.h" // Include for AABB struct definition
#include <iostream> // For debug
#include <limits>   // For std::numeric_limits
#include <algorithm> // For std::min and std::max if needed though glm provides its own

World::World() {
    // Constructor - Now very simple, no OpenGL-dependent calls here.
}

// New init method to be called after OpenGL is ready
void World::init() {
    // Create a concentric area of chunks around world origin.
    // A radius of 1 means a 3x3 area: (-1,0,-1) to (1,0,1)
    int chunkLoadRadius = 1;
    for (int cx = -chunkLoadRadius; cx <= chunkLoadRadius; ++cx) {
        for (int cz = -chunkLoadRadius; cz <= chunkLoadRadius; ++cz) {
            // For now, all chunks are at y=0 chunk coordinate.
            ensureChunkExists(glm::ivec3(cx, 0, cz));
        }
    }
    std::cout << "World: Initialized " << (2*chunkLoadRadius+1)*(2*chunkLoadRadius+1) << " chunks around origin." << std::endl;
}

World::~World() {
    // Destructor - m_chunks with unique_ptr will auto-cleanup
}

bool World::ensureChunkExists(glm::ivec3 chunkCoord) {
    // Temporary: Only allow creation/expansion of chunks on the Y=0 layer for now.
    if (chunkCoord.y != 0) {
        // Optionally, log this attempt if it's useful for debugging player actions.
        // std::cout << "[World::ensureChunkExists] Denied creation of chunk at Y level: " << chunkCoord.y << std::endl;
        return false; // Do not create chunk if not on Y=0 layer.
    }

    if (m_chunks.find(chunkCoord) == m_chunks.end()) {
        m_chunks[chunkCoord] = std::make_unique<Chunk>(chunkCoord);
        // DO NOT call generateSimpleTerrain() or buildMesh() here anymore.
        // Chunk will be processed by processWorldUpdates().
        // std::cout << "World: Created (but not yet generated) chunk at " << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
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
    if (chunk && chunk->isGenerated()) {
        glm::ivec3 localPos = worldBlockToLocalCoord(worldBlockPos);
        return chunk->getBlock(localPos.x, localPos.y, localPos.z);
    }
    return BlockType::Air; // Or some other default if chunk doesn't exist or isn't generated
}

void World::setBlock(glm::ivec3 worldBlockPos, BlockType type) {
    std::cout << "[World::setBlock] Received worldBlockPos: (" 
              << worldBlockPos.x << ", " << worldBlockPos.y << ", " << worldBlockPos.z 
              << ") Type: " << static_cast<int>(type) << std::endl;

    glm::ivec3 chunkCoord = worldBlockToChunkCoord(worldBlockPos);
    std::cout << "    Calculated chunkCoord: (" 
              << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")" << std::endl;

    ensureChunkExists(chunkCoord); // Ensure chunk exists before setting a block
    Chunk* chunk = getChunk(chunkCoord);
    if (chunk) {
        glm::ivec3 localPos = worldBlockToLocalCoord(worldBlockPos);
        std::cout << "    Calculated localPos: (" 
                  << localPos.x << ", " << localPos.y << ", " << localPos.z << ")" << std::endl;
        chunk->setBlock(localPos.x, localPos.y, localPos.z, type);
    }
}

const std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivec3Compare>& World::getLoadedChunks() const {
    return m_chunks;
}

// Implementation of castRay
World::RaycastResult World::castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float maxDistance) const {
    RaycastResult result;
    result.hit = false;

    glm::vec3 normalizedDirection = glm::normalize(rayDirection);
    if (glm::length(normalizedDirection) < 0.0001f) { // Avoid issues with zero direction
        return result; // Cannot cast a ray with no direction
    }

    // Offset origin slightly along direction to avoid self-intersection or issues when flush with a face
    const float originOffset = 0.001f;
    glm::vec3 effectiveRayOrigin = rayOrigin + normalizedDirection * originOffset; // Use normalizedDirection for offset

    glm::ivec3 currentBlockPos = glm::floor(effectiveRayOrigin); 

    glm::vec3 step = glm::sign(normalizedDirection);

    glm::vec3 tMax; // Distance (in units of t) to the next voxel boundary along each axis
    // For X axis:
    if (normalizedDirection.x == 0.0f) tMax.x = std::numeric_limits<float>::infinity();
    else if (step.x > 0) tMax.x = (currentBlockPos.x + 1.0f - effectiveRayOrigin.x) / normalizedDirection.x;
    else tMax.x = (effectiveRayOrigin.x - currentBlockPos.x) / -normalizedDirection.x; // or (currentBlockPos.x - effectiveRayOrigin.x) / normalizedDirection.x
    // For Y axis:
    if (normalizedDirection.y == 0.0f) tMax.y = std::numeric_limits<float>::infinity();
    else if (step.y > 0) tMax.y = (currentBlockPos.y + 1.0f - effectiveRayOrigin.y) / normalizedDirection.y;
    else tMax.y = (effectiveRayOrigin.y - currentBlockPos.y) / -normalizedDirection.y;
    // For Z axis:
    if (normalizedDirection.z == 0.0f) tMax.z = std::numeric_limits<float>::infinity();
    else if (step.z > 0) tMax.z = (currentBlockPos.z + 1.0f - effectiveRayOrigin.z) / normalizedDirection.z;
    else tMax.z = (effectiveRayOrigin.z - currentBlockPos.z) / -normalizedDirection.z;

    glm::vec3 tDelta; // How much t to advance along each axis to cross one voxel boundary
    if (normalizedDirection.x == 0.0f) tDelta.x = std::numeric_limits<float>::infinity(); 
    else tDelta.x = std::abs(1.0f / normalizedDirection.x);
    if (normalizedDirection.y == 0.0f) tDelta.y = std::numeric_limits<float>::infinity(); 
    else tDelta.y = std::abs(1.0f / normalizedDirection.y);
    if (normalizedDirection.z == 0.0f) tDelta.z = std::numeric_limits<float>::infinity(); 
    else tDelta.z = std::abs(1.0f / normalizedDirection.z);

    glm::ivec3 previousBlockPos = currentBlockPos;
    float currentDistance = 0.0f;

    while (currentDistance < maxDistance) {
        previousBlockPos = currentBlockPos;

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                currentBlockPos.x += static_cast<int>(step.x);
                currentDistance = tMax.x;
                tMax.x += tDelta.x;
            } else {
                currentBlockPos.z += static_cast<int>(step.z);
                currentDistance = tMax.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                currentBlockPos.y += static_cast<int>(step.y);
                currentDistance = tMax.y;
                tMax.y += tDelta.y;
            } else {
                currentBlockPos.z += static_cast<int>(step.z);
                currentDistance = tMax.z;
                tMax.z += tDelta.z;
            }
        }

        if (currentDistance >= maxDistance) {
            result.hit = false;
            break;
        }

        BlockType block = getBlock(currentBlockPos);
        if (block != BlockType::Air) {
            result.hit = true;
            result.blockHit = currentBlockPos;
            result.blockBefore = previousBlockPos; // The block right before we hit solid
            // std::cout << "Ray hit at: " << result.blockHit.x << ", " << result.blockHit.y << ", " << result.blockHit.z << std::endl;
            // std::cout << "Block before: " << result.blockBefore.x << ", " << result.blockBefore.y << ", " << result.blockBefore.z << std::endl;
            break;
        }
    }
    return result;
}

void World::processWorldUpdates() {
    // Process one chunk generation per call
    for (auto& pair : m_chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && !chunk->isGenerated()) {
            chunk->generateSimpleTerrain(); // This will set needsMeshBuild to true
            // std::cout << "World processed generation for chunk: " << chunk->getWorldPosition().x << ", " << chunk->getWorldPosition().z << std::endl;
        }
    }

    // Process all mesh builds per call, only for generated chunks
    for (auto& pair : m_chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->isGenerated() && chunk->needsMeshBuild()) {
            chunk->buildMesh();
            // std::cout << "World processed mesh build for chunk: " << chunk->getWorldPosition().x << ", " << chunk->getWorldPosition().z << std::endl;
        }
    }
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

// --- Collision Detection --- 

// Helper to check AABB-AABB intersection
bool checkAABBCollision(const AABB& a, const AABB& b) {
    // Check for overlap on each axis
    bool overlapX = a.min.x < b.max.x && a.max.x > b.min.x;
    bool overlapY = a.min.y < b.max.y && a.max.y > b.min.y;
    bool overlapZ = a.min.z < b.max.z && a.max.z > b.min.z;
    return overlapX && overlapY && overlapZ;
}

// Main collision resolution function
bool World::resolveCollisions(AABB& playerAABB, glm::vec3& playerVelocity, bool& io_isOnGround) {
    bool collisionOccurredOverall = false;
    io_isOnGround = false; // Assume not on ground until a supporting collision is found

    // Iterate multiple times to handle complex collisions and settling
    const int collisionPasses = 3; // Can be tuned

    for (int pass = 0; pass < collisionPasses; ++pass) {
        bool collisionThisPass = false;

        glm::ivec3 minBlock = glm::floor(playerAABB.min - glm::vec3(1.0f)); // Expand search slightly
        glm::ivec3 maxBlock = glm::ceil(playerAABB.max + glm::vec3(1.0f));  // Expand search slightly

        for (int y = minBlock.y; y <= maxBlock.y; ++y) {
            for (int x = minBlock.x; x <= maxBlock.x; ++x) {
                for (int z = minBlock.z; z <= maxBlock.z; ++z) {
                    glm::ivec3 currentBlockPos(x, y, z);
                    BlockType blockType = getBlock(currentBlockPos);

                    if (blockType != BlockType::Air) {
                        AABB blockAABB;
                        blockAABB.min = glm::vec3(currentBlockPos);
                        blockAABB.max = glm::vec3(currentBlockPos) + glm::vec3(1.0f);

                        if (checkAABBCollision(playerAABB, blockAABB)) {
                            collisionThisPass = true;
                            collisionOccurredOverall = true;

                            // Calculate overlap (penetration) on each axis
                            float overlapX = 0.0f;
                            if (playerVelocity.x > 0) overlapX = blockAABB.min.x - playerAABB.max.x;
                            else if (playerVelocity.x < 0) overlapX = blockAABB.max.x - playerAABB.min.x;
                            
                            float overlapY = 0.0f;
                            if (playerVelocity.y > 0) overlapY = blockAABB.min.y - playerAABB.max.y;
                            else if (playerVelocity.y < 0) overlapY = blockAABB.max.y - playerAABB.min.y;

                            float overlapZ = 0.0f;
                            if (playerVelocity.z > 0) overlapZ = blockAABB.min.z - playerAABB.max.z;
                            else if (playerVelocity.z < 0) overlapZ = blockAABB.max.z - playerAABB.min.z;

                            // Calculate how far to move playerAABB back in time along velocity vector to just touch
                            // This is a more continuous approach but simpler is often better to start.
                            // For now, let's use a simpler "push-out" based on minimum penetration.

                            glm::vec3 penetration;
                            // Corrected penetration calculation:
                            // Amount of overlap = (sum of half-widths) - (distance between centers)
                            // Simpler: dist between centers = max(a.min, b.min) - min(a.max, b.max)
                            // More robust penetration: (ref: https://gamedev.stackexchange.com/a/29796)
                            glm::vec3 centerA = (playerAABB.min + playerAABB.max) * 0.5f;
                            glm::vec3 centerB = (blockAABB.min + blockAABB.max) * 0.5f;
                            glm::vec3 halfSizeA = (playerAABB.max - playerAABB.min) * 0.5f;
                            glm::vec3 halfSizeB = (blockAABB.max - blockAABB.min) * 0.5f;

                            glm::vec3 delta = centerA - centerB;
                            penetration.x = (halfSizeA.x + halfSizeB.x) - glm::abs(delta.x);
                            penetration.y = (halfSizeA.y + halfSizeB.y) - glm::abs(delta.y);
                            penetration.z = (halfSizeA.z + halfSizeB.z) - glm::abs(delta.z);

                            // Determine the side of collision and resolve
                            // This finds the axis of minimum penetration to resolve first.
                            if (penetration.y < penetration.x && penetration.y < penetration.z) {
                                // Vertical collision resolution
                                if (playerAABB.min.y < blockAABB.min.y) { // Player is below block (ceiling collision)
                                    playerAABB.min.y -= penetration.y;
                                    playerAABB.max.y -= penetration.y;
                                    if (playerVelocity.y > 0) playerVelocity.y = 0; // Hit ceiling
                                } else { // Player is above block (floor collision)
                                    playerAABB.min.y += penetration.y;
                                    playerAABB.max.y += penetration.y;
                                    if (playerVelocity.y < 0) {
                                        playerVelocity.y = 0; // Landed on ground
                                        io_isOnGround = true;
                                    }
                                }
                            } else if (penetration.x < penetration.y && penetration.x < penetration.z) {
                                // Horizontal X collision resolution
                                if (playerAABB.min.x < blockAABB.min.x) { // Player is to the left of block
                                    playerAABB.min.x -= penetration.x;
                                    playerAABB.max.x -= penetration.x;
                                } else { // Player is to the right of block
                                    playerAABB.min.x += penetration.x;
                                    playerAABB.max.x += penetration.x;
                                }
                                if ((playerVelocity.x > 0 && playerAABB.max.x > blockAABB.min.x) || (playerVelocity.x < 0 && playerAABB.min.x < blockAABB.max.x)) {
                                   playerVelocity.x = 0; 
                                }
                            } else {
                                // Horizontal Z collision resolution
                                if (playerAABB.min.z < blockAABB.min.z) { // Player is in front of block
                                    playerAABB.min.z -= penetration.z;
                                    playerAABB.max.z -= penetration.z;
                                } else { // Player is behind block
                                    playerAABB.min.z += penetration.z;
                                    playerAABB.max.z += penetration.z;
                                }
                                 if ((playerVelocity.z > 0 && playerAABB.max.z > blockAABB.min.z) || (playerVelocity.z < 0 && playerAABB.min.z < blockAABB.max.z)) {
                                   playerVelocity.z = 0; 
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!collisionThisPass) break; // If no collisions in a pass, we are stable
    }
    return collisionOccurredOverall;
} 