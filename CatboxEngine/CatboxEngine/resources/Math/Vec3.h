#pragma once

struct Vec3
{
    float x = 0;
    float y = 0;
    float z = 0;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& rhs) const {
        return { x + rhs.x, y + rhs.y, z + rhs.z };
    }

    Vec3 operator*(float s) const {
        return { x * s, y * s, z * s };
    }
};
