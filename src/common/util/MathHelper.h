#pragma once
#include <cmath>

namespace math
{
    inline float square(float v)
    {
        return v * v;
    }

    inline float distance2dSq(float x, float y)
    {
        return square(x) + square(y);
    }

    inline float distance2d(float x, float y) 
    {
        return std::sqrt(distance2dSq(x, y));
    }

    inline float distance3dSq(float x, float y, float z)
    {
        return distance2dSq(x, y) + square(z);
    }

    inline float distance3d(float x, float y, float z)
    {
        return std::sqrt(distance3dSq(x, y, z));
    }

    inline float slope2d(float x1, float x2, float y1, float y2)
    {
        return (y2 - y1) / (x2 - x1);
    }

    // gets the slope of z from the x/y plane
    inline float slope3d(float x1, float x2, float y1, float y2, float z1, float z2)
    {
        return (z2 - z1) / distance2d(x2 - x1, y2 - y1);
    }
}