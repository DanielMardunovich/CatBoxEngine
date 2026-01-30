#pragma once
#include "../resources/Math/Vec3.h"

struct Transform 
{
    Vec3 Position { 0.0f, 0.0f, 0.0f };
    Vec3 Rotation { 0.0f, 0.0f, 0.0f };
    Vec3 Scale    { 1.0f, 1.0f, 1.0f };

    // Default constructor
    constexpr Transform() noexcept = default;

    // Constructor with position only
    constexpr explicit Transform(const Vec3& position) noexcept
        : Position(position), Rotation(0.0f, 0.0f, 0.0f), Scale(1.0f, 1.0f, 1.0f)
    {
    }

    // Constructor with position, rotation, and scale
    constexpr Transform(const Vec3& position, const Vec3& rotation, const Vec3& scale) noexcept
        : Position(position), Rotation(rotation), Scale(scale)
    {
    }

    // Reset to identity
    void Reset() noexcept
    {
        Position = Vec3(0.0f, 0.0f, 0.0f);
        Rotation = Vec3(0.0f, 0.0f, 0.0f);
        Scale = Vec3(1.0f, 1.0f, 1.0f);
    }

    // Translate by offset
    void Translate(const Vec3& offset) noexcept
    {
        Position += offset;
    }

    // Rotate by angles (in degrees)
    void Rotate(const Vec3& angles) noexcept
    {
        Rotation += angles;
    }

    // Scale uniformly
    void ScaleUniform(float factor) noexcept
    {
        Scale *= factor;
    }
};
