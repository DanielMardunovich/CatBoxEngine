#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glfw3.h>
#include <glm/glm.hpp>

// keyboard movement
void Camera::Update(GLFWwindow* window, float deltaTime)
{
    float speed = 2.5f * deltaTime;

    glm::vec3 camPos(Position.x, Position.y, Position.z);
    glm::vec3 camFront(Front.x, Front.y, Front.z);
    glm::vec3 camUpVec(Up.x, Up.y, Up.z);

    glm::vec3 forward = glm::normalize(camFront);
    glm::vec3 right = glm::normalize(glm::cross(forward, camUpVec));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += forward * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= forward * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camPos += camUpVec * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos -= camUpVec * speed;

    Position = { camPos.x, camPos.y, camPos.z };
}

void Camera::OnMouseMove(double xpos, double ypos)
{
    if (!cursorCaptured) return;
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    ProcessMouseMovement(xoffset, yoffset);
}

void Camera::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
        cursorCaptured = true;
    }
    else if (action == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
        cursorCaptured = false;
    }
}


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
