#include "World.h"
#include <iostream> // For debug
#include <limits>   // For std::numeric_limits

World::World() {
    // Constructor - Now very simple, no OpenGL-dependent calls here.
}

// New init method to be called after OpenGL is ready
void World::init() {
    // Create a 2x1x2 area of chunks around world origin for a reasonable start.
    // Chunk coordinates will be (0,0,0), (1,0,0), (0,0,1), (1,0,1)
    for (int x = 0; x <= 1; ++x) { // Changed from -1 to 1  => 0 to 1 (for a 2x2 initial area starting at 0,0)
        for (int z = 0; z <= 1; ++z) { // Changed from -1 to 1 => 0 to 1
            ensureChunkExists(glm::ivec3(x, 0, z));
        }
    }
    // For a single chunk at origin for fastest loading:
    // ensureChunkExists(glm::ivec3(0, 0, 0));
}

World::~World() {
    // Destructor - m_chunks with unique_ptr will auto-cleanup
}

bool World::ensureChunkExists(glm::ivec3 chunkCoord) {
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
    const float originOffset = 0.001f; // Increased from 1e-4f for potentially more robust boundary handling
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