#pragma once
#include "Math/Vec3.h"
#include <glm/glm.hpp>

struct GLFWwindow;

// Frustum planes for culling
struct Frustum
{
    glm::vec4 planes[6];  // Left, Right, Bottom, Top, Near, Far
    
    // Check if AABB (bounding box) intersects frustum
    bool IsBoxVisible(const Vec3& min, const Vec3& max) const;
};

struct Camera
{
    // Position and orientation
    Vec3 Position { 0.0f, 0.0f, 3.0f };
    Vec3 Target { 0.0f, 0.0f, 0.0f };
    Vec3 Front { 0.0f, 0.0f, -1.0f };
    Vec3 Up { 0.0f, 1.0f, 0.0f };
    
    // Euler angles
    float Yaw = -90.0f;   // Pointing towards -Z
    float Pitch = 0.0f;
    
    // Camera settings
    float MouseSensitivity = 0.1f;
    float FOV = 60.0f;
    float Aspect = 1.0f;
    float Near = 0.1f;
    float Far = 100.0f;

    // Setters
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

    // Mouse input processing
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    
    // Update camera (keyboard movement)
    void Update(GLFWwindow* window, float deltaTime);

    // Event callbacks
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(GLFWwindow* window, int button, int action, int mods);

    // Initialize camera fields and compute initial orientation
    void Initialize(const Vec3& position, const Vec3& target, const Vec3& up,
                    float fovDegrees, float aspect, float nearPlane, float farPlane);

    // Matrix getters
    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
    
    // Frustum culling
    [[nodiscard]] Frustum GetFrustum() const;
    [[nodiscard]] bool IsBoxInFrustum(const Vec3& min, const Vec3& max) const;

private:
    // Internal mouse state
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
    bool m_firstMouse = true;
    bool m_cursorCaptured = false;
};
