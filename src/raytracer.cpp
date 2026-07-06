/**
 * raytracer.cpp
 * Implementação do engine de Ray Tracing em C++.
 *
 * Técnicas implementadas:
 *  - Ray casting com câmera perspectiva
 *  - Interseção ray-esfera (equação quadrática)
 *  - Modelo de iluminação de Phong (ambiente + difuso + especular)
 *  - Sombras duras (shadow rays)
 *  - Reflexões recursivas
 *  - Anti-aliasing por supersampling (amostras aleatórias por pixel)
 *
 * Esta unidade é compilada como DLL/SO para ser consumida
 * por outras linguagens (Python/ctypes) sem recompilação.
 */

#include "raytracer.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <limits>

// ============================================================
//  Constantes e auxiliares
// ============================================================

static const float T_MIN  = 1e-4f;
static const float T_MAX  = 1e+6f;
static const int   MAX_DEPTH = 4;

/** Número aleatório em [0, 1). */
static inline float frand() {
    return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f);
}

/** Clamp de float para [0, 1]. */
static inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

// ============================================================
//  Ray: raio com origem e direção normalizada
// ============================================================

struct Ray {
    Vec3 origin;
    Vec3 dir;

    /** Ponto ao longo do raio no parâmetro t. */
    Vec3 at(float t) const { return origin + dir * t; }
};

// ============================================================
//  Cena (esfera de Cornell Box simplificada)
// ============================================================

/** Resultado de uma interseção ray-esfera. */
struct HitRecord {
    float t;
    Vec3  point;
    Vec3  normal;
    Material mat;
    bool  hit;
};

/**
 * intersect_sphere: testa a interseção entre um raio e uma esfera.
 * Resolve a equação quadrática |P + t*d - C|² = r².
 *
 * @return HitRecord com hit=true e t no intervalo [tmin, tmax], ou hit=false.
 */
static HitRecord intersect_sphere(const Ray& ray, const Sphere& s, float tmin, float tmax) {
    Vec3  oc = ray.origin - s.center;
    float a  = ray.dir.dot(ray.dir);
    float b  = oc.dot(ray.dir);
    float c  = oc.dot(oc) - s.radius * s.radius;
    float disc = b * b - a * c;

    HitRecord rec;
    rec.hit = false;

    if (disc < 0.f) return rec;

    float sqrt_disc = sqrtf(disc);
    float t = (-b - sqrt_disc) / a;
    if (t < tmin || t > tmax) {
        t = (-b + sqrt_disc) / a;
        if (t < tmin || t > tmax) return rec;
    }

    rec.hit    = true;
    rec.t      = t;
    rec.point  = ray.at(t);
    rec.normal = (rec.point - s.center) / s.radius;
    rec.mat    = s.mat;
    return rec;
}

// ============================================================
//  Definição da cena (Cornell Box simplificada)
// ============================================================

static const int NUM_SPHERES = 7;
static Sphere scene_spheres[NUM_SPHERES];
static Light  scene_light;

static void build_scene() {
    // Plano do chão (esfera gigante)
    scene_spheres[0] = { {0, -1000.5f, 0}, 1000.f, {{0.6f, 0.6f, 0.6f}, 0.05f, 16.f} };

    // Esfera central — vermelho brilhante com reflexo
    scene_spheres[1] = { {0, 0, -2}, 0.5f, {{0.9f, 0.15f, 0.1f}, 0.35f, 64.f} };

    // Esfera azul à esquerda
    scene_spheres[2] = { {-1.1f, 0, -2}, 0.5f, {{0.1f, 0.4f, 0.9f}, 0.15f, 32.f} };

    // Esfera dourada à direita
    scene_spheres[3] = { {1.1f, 0, -2}, 0.5f, {{0.95f, 0.75f, 0.1f}, 0.55f, 128.f} };

    // Esfera espelhada no fundo
    scene_spheres[4] = { {0, 0, -3.5f}, 0.7f, {{0.9f, 0.9f, 0.9f}, 0.95f, 256.f} };

    // Esfera verde pequena
    scene_spheres[5] = { {-0.5f, -0.25f, -1.2f}, 0.25f, {{0.2f, 0.85f, 0.3f}, 0.1f, 16.f} };

    // Esfera roxa pequena
    scene_spheres[6] = { {0.55f, -0.25f, -1.2f}, 0.25f, {{0.6f, 0.1f, 0.9f}, 0.2f, 32.f} };

    // Luz pontual branca acima e à frente
    scene_light = { {2.f, 4.f, 0.f}, {1.f, 1.f, 1.f}, 1.f };
}

// ============================================================
//  Traversal e shading
// ============================================================

/**
 * closest_hit: encontra a esfera mais próxima atingida pelo raio.
 */
static HitRecord closest_hit(const Ray& ray, float tmin, float tmax) {
    HitRecord best;
    best.hit = false;
    float closest = tmax;

    for (int i = 0; i < NUM_SPHERES; i++) {
        HitRecord rec = intersect_sphere(ray, scene_spheres[i], tmin, closest);
        if (rec.hit) {
            closest = rec.t;
            best    = rec;
        }
    }
    return best;
}

/**
 * in_shadow: verifica se o ponto está em sombra em relação à luz.
 */
static bool in_shadow(const Vec3& point) {
    Vec3 to_light = scene_light.pos - point;
    float dist    = to_light.length();
    Ray   shadow  = { point, to_light.normalize() };
    HitRecord rec = closest_hit(shadow, T_MIN, dist);
    return rec.hit;
}

/**
 * shade: calcula a cor do ponto atingido usando Phong + sombras + reflexões.
 *
 * @param ray    raio que atingiu o ponto
 * @param rec    registro de interseção
 * @param depth  profundidade recursiva atual
 */
static Vec3 shade(const Ray& ray, const HitRecord& rec, int depth);

/**
 * trace: traça um raio pela cena e retorna a cor resultante.
 */
static Vec3 trace(const Ray& ray, int depth) {
    if (depth > MAX_DEPTH) return {0, 0, 0};

    HitRecord rec = closest_hit(ray, T_MIN, T_MAX);

    if (rec.hit) return shade(ray, rec, depth);

    // Gradiente de céu (azul → branco) quando o raio não acerta nada
    float t = 0.5f * (ray.dir.normalize().y + 1.0f);
    Vec3 sky_top    = {0.35f, 0.55f, 0.9f};
    Vec3 sky_bottom = {1.f, 1.f, 1.f};
    return sky_bottom * (1.f - t) + sky_top * t;
}

static Vec3 shade(const Ray& ray, const HitRecord& rec, int depth) {
    Vec3 N     = rec.normal.normalize();
    Vec3 to_l  = (scene_light.pos - rec.point).normalize();
    Vec3 to_c  = (ray.origin - rec.point).normalize();

    // Ambiente
    Vec3 ambient = rec.mat.color * 0.12f;

    // Difuso (Lambert)
    float diff_f = std::max(0.f, N.dot(to_l));
    Vec3  diffuse = rec.mat.color * diff_f * 0.75f;

    // Especular (Phong)
    Vec3  R        = (to_l * -1.f).reflect(N);
    float spec_f   = powf(std::max(0.f, to_c.dot(R)), rec.mat.shininess);
    Vec3  specular = scene_light.color * spec_f * 0.6f;

    // Sombra: remove difuso e especular se bloqueado
    if (in_shadow(rec.point)) {
        diffuse  = {0, 0, 0};
        specular = {0, 0, 0};
    }

    Vec3 local = ambient + diffuse + specular;

    // Reflexão recursiva
    if (rec.mat.reflectivity > 0.01f && depth < MAX_DEPTH) {
        Vec3 ref_dir  = ray.dir.reflect(N);
        Ray  ref_ray  = { rec.point, ref_dir.normalize() };
        Vec3 ref_col  = trace(ref_ray, depth + 1);
        local = local * (1.f - rec.mat.reflectivity) + ref_col * rec.mat.reflectivity;
    }

    return local;
}

// ============================================================
//  Câmera perspectiva simples
// ============================================================

static Camera make_camera(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    float vfov   = 60.f * (3.14159265f / 180.f); // 60° em radianos
    float half_h = tanf(vfov / 2.f);
    float half_w = aspect * half_h;

    Camera cam;
    cam.origin           = {0, 0, 1};
    cam.lower_left_corner = {-half_w, -half_h, 0};
    cam.horizontal       = {2.f * half_w, 0, 0};
    cam.vertical         = {0, 2.f * half_h, 0};
    return cam;
}

static Ray camera_ray(const Camera& cam, float u, float v) {
    Vec3 dir = cam.lower_left_corner
             + cam.horizontal * u
             + cam.vertical   * v
             - cam.origin;
    return { cam.origin, dir.normalize() };
}

// ============================================================
//  Ponto de entrada exportado
// ============================================================

/**
 * render: renderiza a cena inteira e grava em out_rgb.
 * Ordem do buffer: linha a linha de cima para baixo, R G B por pixel.
 */
extern "C" DLL_EXPORT void render(int width, int height, int samples, uint8_t* out_rgb) {
    build_scene();
    Camera cam = make_camera(width, height);

    for (int j = height - 1; j >= 0; j--) {
        for (int i = 0; i < width; i++) {
            Vec3 color = {0, 0, 0};

            // Supersampling: média de `samples` raios por pixel
            for (int s = 0; s < samples; s++) {
                float u = (i + frand()) / static_cast<float>(width);
                float v = (j + frand()) / static_cast<float>(height);
                Ray   r = camera_ray(cam, u, v);
                Vec3  c = trace(r, 0);
                color   = color + c;
            }

            color = color / static_cast<float>(samples);

            // Correção gama (γ = 2.2)
            color.x = powf(clamp01(color.x), 1.f / 2.2f);
            color.y = powf(clamp01(color.y), 1.f / 2.2f);
            color.z = powf(clamp01(color.z), 1.f / 2.2f);

            // Conversão para [0, 255] e escrita no buffer
            int idx = ((height - 1 - j) * width + i) * 3;
            out_rgb[idx + 0] = static_cast<uint8_t>(color.x * 255.99f);
            out_rgb[idx + 1] = static_cast<uint8_t>(color.y * 255.99f);
            out_rgb[idx + 2] = static_cast<uint8_t>(color.z * 255.99f);
        }
    }
}
