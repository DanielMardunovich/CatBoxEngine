#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::GetViewMatrix() const
{
    glm::vec3 pos(Position.x, Position.y, Position.z);
    glm::vec3 front(Front.x, Front.y, Front.z);
    glm::vec3 up(Up.x, Up.y, Up.z);
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(FOV), Aspect, Near, Far);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    // update Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front = glm::normalize(front);
    Front = { front.x, front.y, front.z };
}
