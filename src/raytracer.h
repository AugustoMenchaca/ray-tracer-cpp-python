/**
 * raytracer.h
 * Engine de Ray Tracing implementado em C++.
 *
 * Exporta a função `render` como símbolo C para ser consumida
 * por outras linguagens via FFI (ex: Python/ctypes).
 */

#pragma once

#include <cstdint>
#include <cmath>

// --- Primitivos Matemáticos ---

/**
 * Vec3: vetor/ponto tridimensional de ponto flutuante.
 * Usado para posições, direções, cores e normais.
 */
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float t)       const { return {x * t,   y * t,   z * t};   }
    Vec3 operator*(const Vec3& b) const { return {x * b.x, y * b.y, z * b.z}; }
    Vec3 operator/(float t)       const { return {x / t,   y / t,   z / t};   }

    float dot(const Vec3& b)  const { return x * b.x + y * b.y + z * b.z; }
    float length()            const { return sqrtf(dot(*this)); }
    Vec3  normalize()         const { float l = length(); return {x/l, y/l, z/l}; }
    Vec3  reflect(const Vec3& n) const { return *this - n * (2.0f * dot(n)); }
};

// --- Definições de Cena ---

/**
 * Material: propriedades de superfície de um objeto.
 */
struct Material {
    Vec3  color;        // cor base (albedo)
    float reflectivity; // 0 = opaco, 1 = espelho perfeito
    float shininess;    // expoente especular de Phong
};

/**
 * Sphere: esfera definida por centro, raio e material.
 */
struct Sphere {
    Vec3     center;
    float    radius;
    Material mat;
};

/**
 * Light: fonte de luz pontual.
 */
struct Light {
    Vec3  pos;
    Vec3  color;
    float intensity;
};

/**
 * Camera: define a posição, alvo e campo de visão da câmera.
 */
struct Camera {
    Vec3  origin;   // posição da câmera
    Vec3  lower_left_corner;
    Vec3  horizontal;
    Vec3  vertical;
};

// --- Interface Exportada (símbolo C) ---

#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif

extern "C" {
    /**
     * render: renderiza a cena e grava o resultado no buffer fornecido.
     *
     * @param width    largura da imagem em pixels
     * @param height   altura da imagem em pixels
     * @param samples  número de amostras por pixel (anti-aliasing)
     * @param out_rgb  buffer de saída: width * height * 3 bytes (R, G, B)
     */
    DLL_EXPORT void render(int width, int height, int samples, uint8_t* out_rgb);
}
