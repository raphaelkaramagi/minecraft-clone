#ifndef RENDERER_H
#define RENDERER_H

#include "Shader.h" // Include Shader header
#include "Camera.h" // Include Camera header
#include "Chunk.h" // Include Chunk header for drawChunk
#include <glad/glad.h> // For GLuint
#include <glm/mat4x4.hpp> // For glm::mat4

struct GLFWwindow; // Forward declaration to match GLFW's actual type (struct)

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initializes the renderer (e.g., sets up OpenGL states)
    bool init(GLFWwindow* window);

    // Clears the screen
    void clear() const;

    // Prepares for a new frame (e.g., clear color/depth buffers)
    void beginFrame(const Camera& camera); // Pass camera for view/projection setup once per frame

    // Ends the current frame (placeholder for now)
    void endFrame();

    // Swaps buffers (if an external window is not handling it)
    // For now, main.cpp handles buffer swapping via glfwSwapBuffers

    // Draw a whole chunk
    void drawChunk(const Chunk& chunk);

    void shutdown(); // Added missing declaration

private:
    GLFWwindow* p_window; // Pointer to the GLFW window
    Shader      m_simpleShader;
    glm::mat4   m_viewMatrix;
    glm::mat4   m_projectionMatrix;
    // We might add viewport dimensions, clear color etc. here later
};

#endif // RENDERER_H 