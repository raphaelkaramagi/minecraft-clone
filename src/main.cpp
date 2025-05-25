#define GLFW_INCLUDE_NONE // IMPORTANT: Prevents GLFW from including gl.h or similar
#include <iostream>

// GLFW - Must be included before GLAD
#include <GLFW/glfw3.h>

// GLAD - Must be included after GLFW
#include <glad/glad.h> // We will set up GLAD later

#include "Renderer.h" // Include our new Renderer header
#include "Camera.h" // Include Camera header
#include "World.h" // Include World header

// Globals for window size (for framebuffer_size_callback)
int g_windowWidth = 800;
int g_windowHeight = 600;
Camera g_camera(glm::vec3(0.0f, Chunk::CHUNK_HEIGHT + 5.0f, 30.0f)); // Position camera above origin looking at chunks

// Mouse input state
bool g_firstMouse = true;
float g_lastX = 400, g_lastY = 300;

// Timing
float g_deltaTime = 0.0f;
float g_lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    g_windowWidth = width;
    g_windowHeight = height;
    g_camera.WindowWidth = width;
    g_camera.WindowHeight = height;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (g_firstMouse) {
        g_lastX = xpos;
        g_lastY = ypos;
        g_firstMouse = false;
    }

    float xoffset = xpos - g_lastX;
    float yoffset = g_lastY - ypos; // Reversed since y-coordinates go from bottom to top

    g_lastX = xpos;
    g_lastY = ypos;

    g_camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    g_camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::FORWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::BACKWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::LEFT, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::RIGHT, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) // Example: Space for UP
        g_camera.ProcessKeyboard(Camera_Movement::UP, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) // Example: Shift for DOWN
        g_camera.ProcessKeyboard(Camera_Movement::DOWN, g_deltaTime);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Specify OpenGL version and profile (e.g., 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Create a windowed mode window and its OpenGL context
    // We'll make this configurable later
    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, "Minecraft Clone", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // Set the resize callback
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate(); // Terminate GLFW before returning
        return -1;
    }
    glViewport(0, 0, g_windowWidth, g_windowHeight); // Set viewport after GLAD is initialized

    std::cout << "GLFW Initialized and Window Created!" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Create and initialize Renderer
    Renderer renderer;
    if (!renderer.init(window)) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    World world; // Create the world object

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;

        processInput(window);

        // Camera is updated by mouse_callback and processInput directly

        // Rendering
        renderer.beginFrame(g_camera);

        // Iterate through loaded chunks and draw their meshes
        const auto& loadedChunks = world.getLoadedChunks();
        for (const auto& pair : loadedChunks) {
            const Chunk* chunk = pair.second.get();
            if (chunk) { // Make sure chunk is not null
                renderer.drawChunk(*chunk);
            }
        }

        renderer.endFrame();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Cleanup
    renderer.shutdown(); // Call renderer shutdown
    glfwTerminate();
    return 0;
} 