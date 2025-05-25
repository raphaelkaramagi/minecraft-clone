#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h> // For GL types if needed, and for function loading
#include <glm/mat4x4.hpp> // For glm::mat4

// Forward declarations
struct GLFWwindow;
class Shader;
class Camera;
class Chunk;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(int windowWidth, int windowHeight, struct GLFWwindow* windowHandle);
    void beginFrame(const Camera& camera); // Takes camera for view/projection
    void drawChunk(const Chunk& chunk);    // New method to draw a pre-built chunk mesh
    void drawBlockOutline(const glm::ivec3& blockWorldPos); // For targeted block
    void drawCrosshair(); // For aiming reticle
    void endFrame();
    void cleanup();

    void setViewport(int x, int y, int width, int height);

private:
    Shader* m_shader; // Main 3D shader
    Shader* m_crosshairShader; // Shader for the 2D crosshair

    // VAO/VBO for block outline (a unit cube wireframe)
    GLuint m_outlineVAO;
    GLuint m_outlineVBO;
    // VAO/VBO for crosshair (two short lines)
    GLuint m_crosshairVAO;
    GLuint m_crosshairVBO;

    glm::mat4 m_viewMatrix;
    // struct GLFWwindow* m_window; // Not storing window handle for now
};

#endif // RENDERER_H 