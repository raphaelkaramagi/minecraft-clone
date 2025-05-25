#include "Renderer.h"
#include "Shader.h"
#include "Camera.h"
#include "Chunk.h" // Include Chunk for its definition

// GLAD must be included before GLFW if we need GL functions here
// For now, we only need GLFW for window pointer and glad for glClear etc.
#include <glad/glad.h>
#include <GLFW/glfw3.h> // For GLFWwindow type, glfwSwapBuffers if we move it here

#include <iostream> // For error reporting
// #include <vector> // No longer needed for blockVertices
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, glm::rotate, glm::scale
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr if needed for uniforms

// blockVertices array removed from here. It's temporarily in Chunk.cpp

Renderer::Renderer() : m_shader(nullptr), m_crosshairShader(nullptr), 
                       m_outlineVAO(0), m_outlineVBO(0),
                       m_crosshairVAO(0), m_crosshairVBO(0),
                       m_viewMatrix(1.0f) {
    // m_blockVAO and m_blockVBO removed
    // m_viewMatrix and m_projectionMatrix initialized by beginFrame
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init(int windowWidth, int windowHeight, GLFWwindow* windowHandle) {
    m_shader = new Shader();
    if (!m_shader || !m_shader->load("shaders/simple.vert", "shaders/simple.frag")) {
        std::cerr << "Renderer Error: Failed to load main shaders!" << std::endl;
        delete m_shader; m_shader = nullptr;
        return false;
    }

    m_crosshairShader = new Shader();
    if (!m_crosshairShader || !m_crosshairShader->load("shaders/crosshair.vert", "shaders/crosshair.frag")) {
        std::cerr << "Renderer Error: Failed to load crosshair shaders!" << std::endl;
        delete m_crosshairShader; m_crosshairShader = nullptr;
        delete m_shader; m_shader = nullptr; // also cleanup main shader if crosshair fails
        return false;
    }

    setViewport(0, 0, windowWidth, windowHeight); 
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND); // For potentially transparent UI elements later
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup VAO/VBO for block outline (wireframe cube)
    float outlineVertices[] = {
        // positions (lines draw 1-2, 2-3, 3-4, 4-1, etc. for each face)
        // We need 12 lines, 2 vertices per line, 3 floats per vertex = 72 floats
        // To draw a cube with lines, we list pairs of vertices for each edge.
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, // bottom face
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, // top face
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, // vertical edges
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };
    glGenVertexArrays(1, &m_outlineVAO);
    glGenBuffers(1, &m_outlineVBO);
    glBindVertexArray(m_outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_outlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(outlineVertices), outlineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Setup VAO/VBO for crosshair (two short lines forming a +)
    float crosshairVertices[] = {
        // Horizontal line
        -0.02f,  0.00f,  // Left point
         0.02f,  0.00f,  // Right point
        // Vertical line
         0.00f, -0.03f,  // Bottom point
         0.00f,  0.03f   // Top point
    };
    glGenVertexArrays(1, &m_crosshairVAO);
    glGenBuffers(1, &m_crosshairVBO);
    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return true;
}

void Renderer::beginFrame(const Camera& camera) {
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // A nice sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_shader) return;

    m_shader->use(); // Use shader before setting uniforms
    m_viewMatrix = camera.GetViewMatrix();
    // Get projection matrix from camera
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix(); 
    
    m_shader->setMat4("view", m_viewMatrix);
    m_shader->setMat4("projection", projectionMatrix); // Use camera's projection matrix
}

void Renderer::endFrame() {
    // Currently, buffer swapping and event polling are handled in main.cpp
}

void Renderer::drawChunk(const Chunk& chunk) {
    if (!m_shader || !chunk.hasMesh() || chunk.getVAO() == 0 || chunk.getVertexCount() == 0) {
        return; 
    }

    m_shader->use(); // Ensure shader is active

    glm::mat4 model = glm::mat4(1.0f);
    // Chunk's worldPosition is already scaled (e.g., (0,0,0), (16,0,0), etc.)
    // Vertices within the chunk mesh are relative to chunk origin (0..15)
    model = glm::translate(model, glm::vec3(chunk.getWorldPosition()));
    
    m_shader->setMat4("model", model);

    glBindVertexArray(chunk.getVAO());
    glDrawArrays(GL_TRIANGLES, 0, chunk.getVertexCount());
    glBindVertexArray(0);
}

void Renderer::drawBlockOutline(const glm::ivec3& blockWorldPos) {
    if (!m_shader || m_outlineVAO == 0) return;

    m_shader->use(); // Use the main 3D shader

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(blockWorldPos)); // Align to blockWorldPos directly
    model = glm::scale(model, glm::vec3(1.002f)); // Slightly scale up
    
    m_shader->setMat4("model", model);

    glDisable(GL_DEPTH_TEST); // Draw outline on top of everything
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Draw in wireframe mode
    glLineWidth(2.0f); // Make lines a bit thicker
    glBindVertexArray(m_outlineVAO);
    glDrawArrays(GL_LINES, 0, 24); // 12 lines * 2 vertices per line
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
    glLineWidth(1.0f); // Reset line width
    glEnable(GL_DEPTH_TEST); // Re-enable depth testing
}

void Renderer::drawCrosshair() {
    if (!m_crosshairShader || m_crosshairVAO == 0) return;

    // Crosshair is 2D, doesn't need view/projection from camera typically
    // It also should be drawn last, without depth testing if it needs to be on top
    glDisable(GL_DEPTH_TEST); // Draw on top of everything
    m_crosshairShader->use();
    glBindVertexArray(m_crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4); // 2 lines * 2 vertices per line
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST); // Re-enable depth testing
}

void Renderer::cleanup() {
    delete m_shader;
    m_shader = nullptr;
    delete m_crosshairShader;
    m_crosshairShader = nullptr;

    if (m_outlineVAO != 0) {
        glDeleteVertexArrays(1, &m_outlineVAO);
        glDeleteBuffers(1, &m_outlineVBO); // Don't forget to delete buffer too
        m_outlineVAO = 0; m_outlineVBO = 0;
    }
    if (m_crosshairVAO != 0) {
        glDeleteVertexArrays(1, &m_crosshairVAO);
        glDeleteBuffers(1, &m_crosshairVBO);
        m_crosshairVAO = 0; m_crosshairVBO = 0;
    }
}

void Renderer::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    // m_projectionMatrix calculation and shader update removed from here.
    // if (height > 0) { 
    //     m_projectionMatrix = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f); 
    //     if (m_shader) { 
    //         m_shader->use();
    //         m_shader->setMat4("projection", m_projectionMatrix);
    //     }
    // }    
} 