#pragma once
#include "../../resources/Math/Vec3.h"

class Camera;

class CameraInspector
{
public:
    CameraInspector() = default;
    ~CameraInspector() = default;

    // Disable copy, enable move
    CameraInspector(const CameraInspector&) = delete;
    CameraInspector& operator=(const CameraInspector&) = delete;
    CameraInspector(CameraInspector&&) noexcept = default;
    CameraInspector& operator=(CameraInspector&&) noexcept = default;

    void Draw(Camera& camera);
};
