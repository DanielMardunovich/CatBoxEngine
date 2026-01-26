#pragma once
#include "Math/Vec3.h"
#include <glm/glm.hpp>

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

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
};
