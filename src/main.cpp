#define GLFW_INCLUDE_NONE // IMPORTANT: Prevents GLFW from including gl.h or similar
#include <iostream>
#include <sstream> // For formatting strings for debug output
#include <iomanip> // For std::setprecision and std::fixed

// GLFW - Must be included before GLAD
#include <GLFW/glfw3.h>

// GLAD - Must be included after GLFW
#include <glad/glad.h> // We will set up GLAD later

#include "Renderer.h" // Include our new Renderer header
#include "Camera.h" // Include Camera header
#include "World.h" // Include World header
#include "TextRenderer.h" // Include TextRenderer header

// Make World and Renderer instances global for access in callbacks for now
// This is not ideal for large projects but simplifies this step.
Renderer g_renderer;
World g_world;
TextRenderer* g_textRenderer = nullptr; // Global TextRenderer pointer
bool g_showDebugInfo = false; // Toggle for F3 debug screen

// Globals for window size (for framebuffer_size_callback)
int g_windowWidth = 800;
int g_windowHeight = 600;

// Simplified direct spawn position for debugging
Camera g_camera(glm::vec3(8.0f, 10.0f, 8.0f), // X=8, Z=8 (center of 0,0,0 chunk). Y=10 to be clearly above ground 
                glm::vec3(0.0f, 1.0f, 0.0f), // Up vector
                YAW, PITCH, // Use default YAW and PITCH from Camera.h
                g_windowWidth, g_windowHeight);

// Mouse input state
bool g_firstMouse = true;
float g_lastX = 400, g_lastY = 300;

// Raycasting distance and target block
const float MAX_RAYCAST_DISTANCE = 5.0f;
World::RaycastResult g_targetedBlock; // Stores the block currently looked at

// Timing
float g_deltaTime = 0.0f;
float g_lastFrame = 0.0f;

// Flight mode globals
const float DOUBLE_TAP_TIME_THRESHOLD = 0.25f; // seconds for double tap
float g_lastSpacePressTime = -1.0f;         // Time of the last spacebar press, init to invalid
const float FLY_SPEED = 5.0f;               // How fast to ascend/descend when flying

// Callback for mouse button events
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Use the continuously updated g_targetedBlock for interactions
        if (g_targetedBlock.hit) {
            std::cout << "Mouse click: Button " << button << " Action " << action << std::endl;
            std::cout << "  Targeted Block: (" << g_targetedBlock.blockHit.x << ", " << g_targetedBlock.blockHit.y << ", " << g_targetedBlock.blockHit.z << ")";
            std::cout << " Before: (" << g_targetedBlock.blockBefore.x << ", " << g_targetedBlock.blockBefore.y << ", " << g_targetedBlock.blockBefore.z << ")" << std::endl;

            if (button == GLFW_MOUSE_BUTTON_LEFT) { 
                // Try to break the block that was hit, if it's not air
                if (g_targetedBlock.hit && g_world.getBlock(g_targetedBlock.blockHit) != BlockType::Air) {
                    g_world.setBlock(g_targetedBlock.blockHit, BlockType::Air);
                }
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) { 
                // Try to place a block in the 'blockBefore' position, 
                // but only if we actually hit a solid block (meaning blockBefore is adjacent air)
                if (g_targetedBlock.hit && g_world.getBlock(g_targetedBlock.blockHit) != BlockType::Air) {
                     // And ensure blockBefore is actually air before placing
                    if (g_world.getBlock(g_targetedBlock.blockBefore) == BlockType::Air) {
                        g_world.setBlock(g_targetedBlock.blockBefore, BlockType::Stone); 
                    }
                }
            }
        } else {
             std::cout << "Mouse click: No target block hit." << std::endl;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_windowWidth = width;
    g_windowHeight = height;

    // Update camera's stored window dimensions for its projection matrix calculation
    if (height > 0) {
        g_camera.WindowWidth = width;
        g_camera.WindowHeight = height;
    }

    // The most direct approach is if Renderer has a global instance or is passed around.
    // Let's assume for now that the projection matrix used by the renderer is updated based on camera in beginFrame.
    // And glViewport should be called here.
    glViewport(0, 0, width, height);

    if (g_textRenderer) { // Update TextRenderer projection
        g_textRenderer->setWindowSize(width, height);
    }

    // The initial renderer.init call also calls setViewport.
    // If Renderer instance is available here:
    // extern Renderer g_renderer_instance; // If it were global
    // g_renderer_instance.setViewport(0,0,width,height); 

    // Tell GLFW to capture our mouse and set mouse button callback
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
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

    // F3 Toggle for Debug Info
    static bool f3_pressed_last_frame = false;
    bool f3_currently_pressed = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;
    if (f3_currently_pressed && !f3_pressed_last_frame) {
        g_showDebugInfo = !g_showDebugInfo;
    }
    f3_pressed_last_frame = f3_currently_pressed;

    // --- Flight and Jump Logic ---
    static bool space_key_physically_down_last_frame = false; // For detecting rising edge of space press
    bool flight_toggled_this_press_event = false;             // True if a double tap toggled flight in this specific press event

    bool space_key_is_currently_pressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    // 1. Handle Double-Tap Toggle on Space Key Press (rising edge)
    if (space_key_is_currently_pressed && !space_key_physically_down_last_frame) {
        float currentTime = static_cast<float>(glfwGetTime());
        if (g_lastSpacePressTime > 0.0f && (currentTime - g_lastSpacePressTime) < DOUBLE_TAP_TIME_THRESHOLD) {
            // Double tap detected
            g_camera.isFlying = !g_camera.isFlying;
            flight_toggled_this_press_event = true; // Mark that flight was toggled by this press
            if (g_camera.isFlying) {
                // g_camera.verticalVelocity = 0.0f; // Stop any fall/jump velocity <-- OLD
                g_camera.Velocity.y = 0.0f; // Stop any fall/jump velocity
                g_camera.isOnGround = false;      // Cannot be on ground if starting to fly
                std::cout << "Flight mode ON (double tap)" << std::endl;
            } else {
                std::cout << "Flight mode OFF (double tap)" << std::endl;
                // Gravity will take over. isOnGround will be updated by physics.
            }
            // Reset lastSpacePressTime to effectively consume the double tap,
            // preventing it from becoming the first tap of another potential double tap immediately.
            g_lastSpacePressTime = 0.0f; 
        } else {
            // Single tap (or first tap of a potential double tap)
            g_lastSpacePressTime = currentTime;
        }
    }
    space_key_physically_down_last_frame = space_key_is_currently_pressed;

    // 2. Handle Continuous Actions (Flying or Jumping) based on current key states
    //    Only if flight wasn't *just* toggled by the most recent press event that occurred on this frame.
    if (!flight_toggled_this_press_event) {
        if (g_camera.isFlying) {
            // Flying controls
            if (space_key_is_currently_pressed) { // Space to ascend
                g_camera.Position.y += FLY_SPEED * g_deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { // Left Shift to descend
                g_camera.Position.y -= FLY_SPEED * g_deltaTime;
            }
        } else {
            // Not flying: Normal jump logic
            if (space_key_is_currently_pressed && g_camera.isOnGround) {
                // g_camera.verticalVelocity = JUMP_FORCE; // JUMP_FORCE from Camera.h <-- OLD
                g_camera.Velocity.y = JUMP_FORCE; // JUMP_FORCE from Camera.h
                g_camera.isOnGround = false;
                // This also serves as the first press action if a double tap doesn't complete.
                // If space is held, subsequent jumps will occur once isOnGround is true again.
            }
        }
    }
    // --- End Flight and Jump Logic ---

    // Horizontal movement - unchanged
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::FORWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::BACKWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::LEFT, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_camera.ProcessKeyboard(Camera_Movement::RIGHT, g_deltaTime);
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

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate(); // Terminate GLFW before returning
        return -1;
    }
    glViewport(0, 0, g_windowWidth, g_windowHeight); // Set viewport after GLAD is initialized

    std::cout << "GLFW Initialized and Window Created!" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize Renderer (now global g_renderer)
    if (!g_renderer.init(g_windowWidth, g_windowHeight, window)) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Initialize TextRenderer
    g_textRenderer = new TextRenderer(g_windowWidth, g_windowHeight);
    if (!g_textRenderer) { // Basic check, TextRenderer constructor should handle its own errors
        std::cerr << "Failed to initialize TextRenderer" << std::endl;
        // Note: TextRenderer constructor doesn't explicitly return bool, relies on cerr for errors
        // We might need a more robust error check from TextRenderer if it can fail init
    }

    // Initialize World (now global g_world, call init after GL is ready)
    g_world.init(); 

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;
        if (g_deltaTime > 0.1f) g_deltaTime = 0.1f; // Clamp max delta time

        glm::vec3 oldCameraPos = g_camera.Position; // Store position before input and y-velocity update

        processInput(window);

        // --- Physics, Movement, and Collision Update ---
        if (!g_camera.isFlying) {
            glm::vec3 displacementThisFrame(0.0f);

            // 1. Apply gravity to vertical velocity
            g_camera.Velocity.y += GRAVITY * g_deltaTime;
            displacementThisFrame.y = g_camera.Velocity.y * g_deltaTime;

            // 2. Get XZ displacement (which was already applied to g_camera.Position by processInput)
            // We need to re-apply it starting from oldCameraPos to combine with Y displacement correctly before collision.
            // So, capture the XZ change that processInput made:
            float dx_from_input = g_camera.Position.x - oldCameraPos.x;
            float dz_from_input = g_camera.Position.z - oldCameraPos.z;
            displacementThisFrame.x = dx_from_input;
            displacementThisFrame.z = dz_from_input;

            // 3. Tentatively update camera position with all displacement components for this frame
            g_camera.Position = oldCameraPos + displacementThisFrame;
            
            // 4. Perform collision detection and resolution
            AABB playerAABB = g_camera.getPlayerAABB();
            glm::vec3 velocityForCollisionResolution = displacementThisFrame / (g_deltaTime > 1e-5f ? g_deltaTime : 1.0f);
            
            g_world.resolveCollisions(playerAABB, velocityForCollisionResolution, g_camera.isOnGround);

            // 5. Update camera position from the (potentially modified) AABB
            g_camera.Position.x = (playerAABB.min.x + playerAABB.max.x) / 2.0f;
            g_camera.Position.y = playerAABB.min.y + PLAYER_EYE_LEVEL;
            g_camera.Position.z = (playerAABB.min.z + playerAABB.max.z) / 2.0f;

            // 6. Update camera's actual Y velocity based on collision outcome
            if (g_camera.isOnGround) {
                if (g_camera.Velocity.y < 0) g_camera.Velocity.y = 0; 
            } else {
                // If hit a ceiling, velocityForCollisionResolution.y would be zeroed by resolveCollisions
                if (velocityForCollisionResolution.y == 0 && (displacementThisFrame.y / (g_deltaTime > 1e-5f ? g_deltaTime : 1.0f)) != 0) {
                    g_camera.Velocity.y = 0;
                }
            }
            // Horizontal velocity is implicitly handled by ProcessKeyboard modifying Position directly each frame.

        } else { // Is Flying - position is directly manipulated by processInput for X,Y,Z flight controls.
            g_camera.isOnGround = false;
            g_camera.Velocity.y = 0.0f; // No gravity or Y-velocity accumulation when flying
        }
        // --- End Physics, Movement, and Collision Update ---

        g_world.processWorldUpdates(); // Process world updates (chunk gen, mesh builds)

        // Continuous raycasting for block highlighting and interaction context
        glm::vec3 rayOrigin = g_camera.Position;
        glm::vec3 rayDirection = g_camera.Front;
        g_targetedBlock = g_world.castRay(rayOrigin, rayDirection, MAX_RAYCAST_DISTANCE);

        // Camera is updated by mouse_callback and processInput directly

        // Rendering
        g_renderer.beginFrame(g_camera); // Use global renderer

        for (const auto& pair : g_world.getLoadedChunks()) { 
            const std::unique_ptr<Chunk>& chunkPtr = pair.second;
            if (chunkPtr && chunkPtr->hasMesh()) { 
                g_renderer.drawChunk(*chunkPtr); 
            }
        }

        // Determine what to outline based on raycast result for interaction context
        bool showOutline = false;
        glm::ivec3 outlinePos;

        if (g_targetedBlock.hit) {
            // If we hit a non-air block, that's our primary target for outlining (for breaking)
            // and blockBefore is where we might place.
            if (g_world.getBlock(g_targetedBlock.blockHit) != BlockType::Air) {
                showOutline = true;
                outlinePos = g_targetedBlock.blockHit; // Outline the solid block itself
            } 
            // If we hit an air block directly (e.g. ray started inside it, or first hit was air - less likely with current raycaster)
            // OR if the ray hit nothing solid but we still want to place based on 'blockBefore' (if ray ended in air next to something)
            // This part needs careful thought. For now, prioritize highlighting the solid block hit.
            // For placement, the outline should be on g_targetedBlock.blockBefore if g_targetedBlock.blockHit is solid.
        }

        if (showOutline) {
            g_renderer.drawBlockOutline(outlinePos);
        }

        g_renderer.drawCrosshair(); 

        // Render Debug Text (F3 screen)
        if (g_showDebugInfo && g_textRenderer) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);

            std::ostringstream oss;
            float yPos = g_windowHeight - 20.0f; // Start from top
            float lineHeight = 20.0f; // Adjust as needed based on font size and scale
            float textScale = 0.7f; // Adjust for desired text size (increased from 0.4f)
            glm::vec3 textColor(1.0f, 1.0f, 1.0f); // White text

            // FPS
            oss << "FPS: " << std::fixed << std::setprecision(1) << (1.0f / g_deltaTime);
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();

            // Player Position
            oss << "XYZ: " << std::fixed << std::setprecision(3) << g_camera.Position.x 
                << " / " << g_camera.Position.y << " / " << g_camera.Position.z;
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();

            // Player Block Position (integer)
            glm::ivec3 playerBlockPos = glm::floor(g_camera.Position);
            oss << "Block: " << playerBlockPos.x << " " << playerBlockPos.y << " " << playerBlockPos.z;
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();

            // Player Chunk Position
            glm::ivec3 playerChunkPos = g_world.worldBlockToChunkCoord(playerBlockPos);
            oss << "Chunk: " << playerChunkPos.x << " " << playerChunkPos.y << " " << playerChunkPos.z;
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();

            // Facing Direction (Simplified)
            // You'd need more complex logic for N/S/E/W from g_camera.Front and Yaw
            oss << "Facing: (see console for Yaw/Pitch)"; // Placeholder
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();
            
            // Targeted Block Info
            if (g_targetedBlock.hit) {
                oss << "Targeted Block: Yes";
                g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, glm::vec3(0.0f,1.0f,0.0f));
                yPos -= lineHeight; oss.str(""); oss.clear();
                oss << "  Hit At: " << g_targetedBlock.blockHit.x << ", " << g_targetedBlock.blockHit.y << ", " << g_targetedBlock.blockHit.z;
                g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, glm::vec3(0.0f,1.0f,0.0f));
                yPos -= lineHeight; oss.str(""); oss.clear();
                BlockType bt = g_world.getBlock(g_targetedBlock.blockHit);
                oss << "  Type: " << static_cast<int>(bt);
                g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, glm::vec3(0.0f,1.0f,0.0f));
                yPos -= lineHeight; oss.str(""); oss.clear();
                 oss << "  Place At: " << g_targetedBlock.blockBefore.x << ", " << g_targetedBlock.blockBefore.y << ", " << g_targetedBlock.blockBefore.z;
                g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, glm::vec3(0.0f,1.0f,0.0f));
                yPos -= lineHeight; oss.str(""); oss.clear();
            } else {
                g_textRenderer->renderText("Targeted Block: No", 10.0f, yPos, textScale, glm::vec3(1.0f,0.0f,0.0f));
                yPos -= lineHeight; oss.str(""); oss.clear();
            }

            // Loaded Chunks Count
            oss << "Loaded Chunks: " << g_world.getLoadedChunks().size();
            g_textRenderer->renderText(oss.str(), 10.0f, yPos, textScale, textColor);
            yPos -= lineHeight; oss.str(""); oss.clear();

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        g_renderer.endFrame(); // Call after all rendering, including text

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Cleanup
    delete g_textRenderer; // Delete TextRenderer
    g_renderer.cleanup(); // Use global renderer
    glfwTerminate();
    return 0;
} 