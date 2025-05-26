#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector> // For storing directions, not strictly needed now but good for future

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum class Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,      // For flying/jumping
    DOWN     // For crouching/descending
};

// Default camera values
const float YAW         = -90.0f; // Initialize yaw to look along negative Z
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;
const float GRAVITY     = -20.0f; // Adjusted for a more game-like feel
const float JUMP_FORCE  =  7.0f; 

// Player Collision Dimensions
const float PLAYER_HEIGHT    = 1.8f;
const float PLAYER_WIDTH     = 0.6f; // Covers X and Z dimensions
const float PLAYER_EYE_LEVEL = 1.6f; // Distance from feet to eye level

struct GLFWwindow; // Forward declaration

// Simple AABB struct
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Camera {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    // Window dimensions for aspect ratio
    int WindowWidth;
    int WindowHeight;

    // Physics related
    glm::vec3 Velocity = glm::vec3(0.0f); // Player's current velocity vector
    bool isOnGround = false;
    bool isFlying = false; // Added for flight mode

    // Helper to get player's collision AABB based on current position
    AABB getPlayerAABB() const {
        // Position is eye level. Calculate feet position first.
        glm::vec3 feetPosition = Position;
        feetPosition.y -= PLAYER_EYE_LEVEL;

        AABB playerBox;
        playerBox.min = glm::vec3(
            feetPosition.x - PLAYER_WIDTH / 2.0f,
            feetPosition.y, // Feet are at the bottom of the AABB
            feetPosition.z - PLAYER_WIDTH / 2.0f
        );
        playerBox.max = glm::vec3(
            feetPosition.x + PLAYER_WIDTH / 2.0f,
            feetPosition.y + PLAYER_HEIGHT, // Top of the AABB
            feetPosition.z + PLAYER_WIDTH / 2.0f
        );
        return playerBox;
    }

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH,
           int windowWidth = 800, int windowHeight = 600) // Add window dimensions here
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        WindowWidth = windowWidth;   // Initialize window dimensions
        WindowHeight = windowHeight; // Initialize window dimensions
        updateCameraVectors();
    }

    // Constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
           float yaw, float pitch,
           int windowWidth = 800, int windowHeight = 600) // Add window dimensions here
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        WindowWidth = windowWidth;   // Initialize window dimensions
        WindowHeight = windowHeight; // Initialize window dimensions
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const;

    glm::mat4 GetProjectionMatrix() const;

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset); // For zooming

private:
    void updateCameraVectors();
};

#endif // CAMERA_H 