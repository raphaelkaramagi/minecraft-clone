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

struct GLFWwindow; // Forward declaration

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