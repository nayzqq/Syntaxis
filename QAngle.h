#pragma once
#include <cmath>

class QAngle
{
public:
    float x, y, z;

    QAngle() : x(0.0f), y(0.0f), z(0.0f) {}
    QAngle(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    QAngle& operator+=(const QAngle& v) { x += v.x; y += v.y; z += v.z; return *this; }
    QAngle& operator-=(const QAngle& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

    void Clamp() {
        while (x > 89.0f) x -= 180.0f;
        while (x < -89.0f) x += 180.0f;
        while (y > 180.0f) y -= 360.0f;
        while (y < -180.0f) y += 360.0f;
        z = 0.0f;
    }
};