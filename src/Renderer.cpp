#include "Renderer.h"

// GLAD must be included before GLFW if we need GL functions here
// For now, we only need GLFW for window pointer and glad for glClear etc.
#include <glad/glad.h>
#include <GLFW/glfw3.h> // For GLFWwindow type, glfwSwapBuffers if we move it here

#include <iostream> // For error reporting
// #include <vector> // No longer needed for blockVertices
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, glm::rotate, glm::scale

// blockVertices array removed from here. It's temporarily in Chunk.cpp

Renderer::Renderer() : p_window(nullptr) {
    // m_blockVAO and m_blockVBO removed
    // m_viewMatrix and m_projectionMatrix initialized by beginFrame
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(GLFWwindow* window) {
    p_window = window;
    if (!p_window) return false;

    if (!m_simpleShader.load("shaders/simple.vert", "shaders/simple.frag")) {
        std::cerr << "Renderer Error: Failed to load simple shader." << std::endl;
        return false;
    }

    // setupBlockMesh(); // Removed - mesh setup is now per-chunk

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    // std::cout << "Renderer initialized." << std::endl; // Less verbose now
    return true;
}

void Renderer::shutdown() {
    // Removed m_blockVBO and m_blockVAO cleanup
    // Shader cleanup is handled by Shader destructor (if implemented)
    // std::cout << "Renderer shutdown." << std::endl;
}

// setupBlockMesh() removed

void Renderer::beginFrame(const Camera& camera) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_viewMatrix = camera.GetViewMatrix();
    m_projectionMatrix = camera.GetProjectionMatrix();
    m_simpleShader.use();
    m_simpleShader.setMat4("view", m_viewMatrix);
    m_simpleShader.setMat4("projection", m_projectionMatrix);
}

void Renderer::endFrame() {
    // Nothing here for now
}

// Replaced drawBlock with drawChunk
void Renderer::drawChunk(const Chunk& chunk) {
    if (!chunk.hasMesh()) return; // Don't draw if chunk has no mesh (e.g. all air)

    glm::mat4 model = glm::mat4(1.0f);
    // The chunk's mesh vertices are already in local chunk coordinates (0-15).
    // We need to translate the whole chunk to its world position.
    model = glm::translate(model, glm::vec3(
        chunk.worldPosition.x * Chunk::CHUNK_WIDTH,
        chunk.worldPosition.y * Chunk::CHUNK_HEIGHT,
        chunk.worldPosition.z * Chunk::CHUNK_DEPTH
    ));
    
    m_simpleShader.setMat4("model", model);
    
    glBindVertexArray(chunk.getVAO());
    glDrawArrays(GL_TRIANGLES, 0, chunk.getVertexCount());
    glBindVertexArray(0); 
} 