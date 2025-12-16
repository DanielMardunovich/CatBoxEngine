#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    enum Movement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 3.0f),
           const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    glm::mat4 GetViewMatrix() const;

    void ProcessKeyboard(Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

    float GetFov() const { return m_Fov; }

    glm::vec3 Position;

private:
    void UpdateCameraVectors();

    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    // Euler angles
    float m_Yaw;
    float m_Pitch;

    // options
    float m_MovementSpeed;
    float m_MouseSensitivity;
    float m_Fov; // field of view in degrees
};