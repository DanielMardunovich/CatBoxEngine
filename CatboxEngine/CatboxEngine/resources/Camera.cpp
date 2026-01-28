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

void Camera::Initialize(const Vec3& position, const Vec3& target, const Vec3& up,
                        float fovDegrees, float aspect, float nearPlane, float farPlane)
{
    Position = position;
    Target = target;
    Up = up;
    FOV = fovDegrees;
    Aspect = aspect;
    Near = nearPlane;
    Far = farPlane;

    // compute initial front vector from target
    glm::vec3 dir = glm::normalize(glm::vec3(target.x - position.x, target.y - position.y, target.z - position.z));
    Front = { dir.x, dir.y, dir.z };

    // compute initial yaw/pitch from front
    Yaw = glm::degrees(atan2(dir.z, dir.x));
    Pitch = glm::degrees(asin(dir.y));
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

// Frustum culling implementation
Frustum Camera::GetFrustum() const
{
    Frustum frustum;
    
    glm::mat4 viewProj = GetProjectionMatrix() * GetViewMatrix();
    
    // Extract frustum planes from view-projection matrix
    // Left plane
    frustum.planes[0] = glm::vec4(
        viewProj[0][3] + viewProj[0][0],
        viewProj[1][3] + viewProj[1][0],
        viewProj[2][3] + viewProj[2][0],
        viewProj[3][3] + viewProj[3][0]
    );
    
    // Right plane
    frustum.planes[1] = glm::vec4(
        viewProj[0][3] - viewProj[0][0],
        viewProj[1][3] - viewProj[1][0],
        viewProj[2][3] - viewProj[2][0],
        viewProj[3][3] - viewProj[3][0]
    );
    
    // Bottom plane
    frustum.planes[2] = glm::vec4(
        viewProj[0][3] + viewProj[0][1],
        viewProj[1][3] + viewProj[1][1],
        viewProj[2][3] + viewProj[2][1],
        viewProj[3][3] + viewProj[3][1]
    );
    
    // Top plane
    frustum.planes[3] = glm::vec4(
        viewProj[0][3] - viewProj[0][1],
        viewProj[1][3] - viewProj[1][1],
        viewProj[2][3] - viewProj[2][1],
        viewProj[3][3] - viewProj[3][1]
    );
    
    // Near plane
    frustum.planes[4] = glm::vec4(
        viewProj[0][3] + viewProj[0][2],
        viewProj[1][3] + viewProj[1][2],
        viewProj[2][3] + viewProj[2][2],
        viewProj[3][3] + viewProj[3][2]
    );
    
    // Far plane
    frustum.planes[5] = glm::vec4(
        viewProj[0][3] - viewProj[0][2],
        viewProj[1][3] - viewProj[1][2],
        viewProj[2][3] - viewProj[2][2],
        viewProj[3][3] - viewProj[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; i++)
    {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= length;
    }
    
    return frustum;
}

bool Camera::IsBoxInFrustum(const Vec3& min, const Vec3& max) const
{
    Frustum frustum = GetFrustum();
    return frustum.IsBoxVisible(min, max);
}

// Frustum AABB intersection test
bool Frustum::IsBoxVisible(const Vec3& min, const Vec3& max) const
{
    // Check box against each frustum plane
    for (int i = 0; i < 6; i++)
    {
        const glm::vec4& plane = planes[i];
        
        // Get positive vertex (farthest in plane normal direction)
        glm::vec3 positive;
        positive.x = (plane.x > 0) ? max.x : min.x;
        positive.y = (plane.y > 0) ? max.y : min.y;
        positive.z = (plane.z > 0) ? max.z : min.z;
        
        // If positive vertex is outside (behind plane), box is outside frustum
        float distance = glm::dot(glm::vec3(plane), positive) + plane.w;
        if (distance < 0)
        {
            return false;  // Box is outside this plane
        }
    }
    
    return true;  // Box intersects or is inside frustum
}
