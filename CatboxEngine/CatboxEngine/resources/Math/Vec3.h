#pragma once

#include <cmath>

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Constructors
    constexpr Vec3() noexcept = default;
    constexpr Vec3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    constexpr explicit Vec3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

    // Arithmetic operators
    [[nodiscard]] constexpr Vec3 operator+(const Vec3& rhs) const noexcept
    {
        return { x + rhs.x, y + rhs.y, z + rhs.z };
    }

    [[nodiscard]] constexpr Vec3 operator-(const Vec3& rhs) const noexcept
    {
        return { x - rhs.x, y - rhs.y, z - rhs.z };
    }

    [[nodiscard]] constexpr Vec3 operator*(float scalar) const noexcept
    {
        return { x * scalar, y * scalar, z * scalar };
    }

    [[nodiscard]] constexpr Vec3 operator/(float scalar) const noexcept
    {
        return { x / scalar, y / scalar, z / scalar };
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept
    {
        return { -x, -y, -z };
    }

    // Compound assignment operators
    constexpr Vec3& operator+=(const Vec3& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vec3& operator*=(float scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3& operator/=(float scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Comparison operators
    [[nodiscard]] constexpr bool operator==(const Vec3& rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec3& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    // Utility functions
    [[nodiscard]] constexpr float Dot(const Vec3& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr Vec3 Cross(const Vec3& other) const noexcept
    {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    [[nodiscard]] float Length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr float LengthSquared() const noexcept
    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] Vec3 Normalized() const noexcept
    {
        const float len = Length();
        if (len > 0.0f)
        {
            return *this / len;
        }
        return *this;
    }

    void Normalize() noexcept
    {
        const float len = Length();
        if (len > 0.0f)
        {
            *this /= len;
        }
    }

    [[nodiscard]] float Distance(const Vec3& other) const noexcept
    {
        return (*this - other).Length();
    }

    [[nodiscard]] constexpr float DistanceSquared(const Vec3& other) const noexcept
    {
        return (*this - other).LengthSquared();
    }
};

// Non-member operators for scalar * Vec3
[[nodiscard]] constexpr Vec3 operator*(float scalar, const Vec3& vec) noexcept
{
    return vec * scalar;
}
