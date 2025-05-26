#include "Camera.h"
// #include <GLFW/glfw3.h> // Only needed if directly using GLFW time, which we are not here.

// Removed problematic constructor that took 'float aspectRatio'
// Constructors from Camera.h will be used.

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    // Use WindowWidth and WindowHeight members as defined in Camera.h
    if (WindowHeight == 0) return glm::mat4(1.0f); // Avoid division by zero
    return glm::perspective(glm::radians(Zoom), (float)WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == Camera_Movement::FORWARD)
        Position += Front * velocity;
    if (direction == Camera_Movement::BACKWARD)
        Position -= Front * velocity;
    if (direction == Camera_Movement::LEFT)
        Position -= Right * velocity;
    if (direction == Camera_Movement::RIGHT)
        Position += Right * velocity;
    // UP and DOWN cases removed as they are handled by physics in main.cpp
    // if (direction == Camera_Movement::UP)      
    //     Position += WorldUp * velocity;     
    // if (direction == Camera_Movement::DOWN)    
    //     Position -= WorldUp * velocity;
    // To keep player on ground, would need to set Position.y = 0.0f; (or some ground level)
    // For true Minecraft-style movement, collision detection with terrain is needed.
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }
    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset; // yoffset is usually amount scrolled
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 75.0f) // Increased max zoom a bit
        Zoom = 75.0f;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}

// Removed SetAspectRatio method as it's redundant and AspectRatio member doesn't exist
// void Camera::SetAspectRatio(float aspectRatio) {
//     AspectRatio = aspectRatio;
// } 