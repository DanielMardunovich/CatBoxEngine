#pragma once

struct Vec4
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;

    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vec4 operator+(const Vec4& rhs) const {
        return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
    }

    Vec4 operator*(float s) const {
        return { x * s, y * s, z * s, w * s };
    }
};
