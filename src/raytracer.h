#pragma once

#include <cstdint>
#include <cmath>

struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float t)       const { return {x * t,   y * t,   z * t};   }
    Vec3 operator*(const Vec3& b) const { return {x * b.x, y * b.y, z * b.z}; }
    Vec3 operator/(float t)       const { return {x / t,   y / t,   z / t};   }

    float dot(const Vec3& b)  const { return x * b.x + y * b.y + z * b.z; }
    float length()            const { return std::sqrt(dot(*this)); }
    Vec3  normalize()         const { float l = length(); return {x/l, y/l, z/l}; }
    Vec3  reflect(const Vec3& n) const { return *this - n * (2.0f * dot(n)); }
};

struct Material {
    Vec3  color;
    float reflectivity;
    float shininess;
};

struct Sphere {
    Vec3     center;
    float    radius;
    Material mat;
};

struct Light {
    Vec3  pos;
    Vec3  color;
    float intensity;
};

struct Camera {
    Vec3  origin;
    Vec3  lower_left_corner;
    Vec3  horizontal;
    Vec3  vertical;
};

#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif

extern "C" {
    DLL_EXPORT void render(int width, int height, int samples, uint8_t* out_rgb);
    DLL_EXPORT void set_scene(int scene_id);
}
