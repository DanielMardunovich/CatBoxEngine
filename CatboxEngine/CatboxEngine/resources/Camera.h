#pragma once
#include "Math/Vec3.h"
struct GLFWwindow;
#include <glm/glm.hpp>

// Frustum planes for culling
struct Frustum
{
    glm::vec4 planes[6];  // Left, Right, Bottom, Top, Near, Far
    
    // Check if AABB (bounding box) intersects frustum
    bool IsBoxVisible(const Vec3& min, const Vec3& max) const;
};

struct Camera
{
    Vec3 Position{0,0,3};
    Vec3 Target{0,0,0};
    Vec3 Front{0,0,-1};
    // Euler angles
    float Yaw = -90.0f; // pointing towards -Z
    float Pitch = 0.0f;

    float MouseSensitivity = 0.1f;
    Vec3 Up{0,1,0};

    float FOV = 60.0f;
    float Aspect = 1.0f;
    float Near = 0.1f;
    float Far = 100.0f;

    void SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        FOV = fovDegrees;
        Aspect = aspect;
        Near = nearPlane;
        Far = farPlane;
    }

    void SetPosition(const Vec3& pos) { Position = pos; }
    void SetTarget(const Vec3& target) { Target = target; }
    void SetUp(const Vec3& up) { Up = up; }

    // Mouse input (xoffset, yoffset) in pixels
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    
    // Update camera (keyboard movement)
    void Update(GLFWwindow* window, float deltaTime);

    // Callbacks
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(GLFWwindow* window, int button, int action, int mods);

    // Initialize camera fields and compute initial orientation
    void Initialize(const Vec3& position, const Vec3& target, const Vec3& up,
                    float fovDegrees, float aspect, float nearPlane, float farPlane);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    
    // Frustum culling
    Frustum GetFrustum() const;
    bool IsBoxInFrustum(const Vec3& min, const Vec3& max) const;

private:
    // internal mouse state
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;
    bool cursorCaptured = false;
};
