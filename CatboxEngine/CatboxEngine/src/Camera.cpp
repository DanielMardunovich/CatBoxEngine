#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera(const glm::vec3& position, const glm::vec3& up, float yaw, float pitch)
    : Position(position),
      m_WorldUp(up),
      m_Yaw(yaw),
      m_Pitch(pitch),
      m_Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_MovementSpeed(2.5f),
      m_MouseSensitivity(0.1f),
      m_Fov(45.0f)
{
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + m_Front, m_Up);
}

void Camera::ProcessKeyboard(Movement direction, float deltaTime)
{
    float velocity = m_MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += m_Front * velocity;
    if (direction == BACKWARD)
        Position -= m_Front * velocity;
    if (direction == LEFT)
        Position -= m_Right * velocity;
    if (direction == RIGHT)
        Position += m_Right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= m_MouseSensitivity;
    yoffset *= m_MouseSensitivity;

    m_Yaw += xoffset;
    m_Pitch += yoffset;

    if (constrainPitch)
    {
        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    m_Fov -= yoffset;
    if (m_Fov < 1.0f) m_Fov = 1.0f;
    if (m_Fov > 90.0f) m_Fov = 90.0f;
}

void Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}