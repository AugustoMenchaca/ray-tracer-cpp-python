#include "raytracer.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static const float T_MIN = 1e-4f;
static const float T_MAX = 1e+6f;
static const int MAX_DEPTH = 4;

static inline float frand() {
    return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f);
}

static inline float clamp01(float v) {
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

struct Ray {
    Vec3 origin;
    Vec3 dir;

    Vec3 at(float t) const { return origin + dir * t; }
};

struct HitRecord {
    float t;
    Vec3  point;
    Vec3  normal;
    Material mat;
    bool  hit;
};

// Faz a logica do raio bater na esfera
static HitRecord intersect_sphere(const Ray& ray, const Sphere& s, float tmin, float tmax) {
    Vec3  oc = ray.origin - s.center;
    float a  = ray.dir.dot(ray.dir);
    float b  = oc.dot(ray.dir);
    float c  = oc.dot(oc) - s.radius * s.radius;
    float disc = b * b - a * c;

    HitRecord rec;
    rec.hit = false;

    if (disc < 0.f) return rec;

    float sqrt_disc = std::sqrt(disc);
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

static const int qtd_spheres = 7;
static Sphere minhasEsferas[qtd_spheres];
static Light  minhaLuz;

static void build_scene() {
    // chao
    minhasEsferas[0] = { {0, -1000.5f, 0}, 1000.f, {{0.6f, 0.6f, 0.6f}, 0.05f, 16.f} };
    // centro vermelho
    minhasEsferas[1] = { {0, 0, -2}, 0.5f, {{0.9f, 0.15f, 0.1f}, 0.35f, 64.f} };
    // azul esquerda
    minhasEsferas[2] = { {-1.1f, 0, -2}, 0.5f, {{0.1f, 0.4f, 0.9f}, 0.15f, 32.f} };
    // dourada direita
    minhasEsferas[3] = { {1.1f, 0, -2}, 0.5f, {{0.95f, 0.75f, 0.1f}, 0.55f, 128.f} };
    // espelho fundo
    minhasEsferas[4] = { {0, 0, -3.5f}, 0.7f, {{0.9f, 0.9f, 0.9f}, 0.95f, 256.f} };
    // verde pequena
    minhasEsferas[5] = { {-0.5f, -0.25f, -1.2f}, 0.25f, {{0.2f, 0.85f, 0.3f}, 0.1f, 16.f} };
    // roxa pequena
    minhasEsferas[6] = { {0.55f, -0.25f, -1.2f}, 0.25f, {{0.6f, 0.1f, 0.9f}, 0.2f, 32.f} };

    minhaLuz = { {2.f, 4.f, 0.f}, {1.f, 1.f, 1.f}, 1.f };
}

static HitRecord closest_hit(const Ray& ray, float tmin, float tmax) {
    HitRecord best;
    best.hit = false;
    float closest = tmax;

    for (int i = 0; i < qtd_spheres; i++) {
        HitRecord rec = intersect_sphere(ray, minhasEsferas[i], tmin, closest);
        if (rec.hit) {
            closest = rec.t;
            best    = rec;
        }
    }
    return best;
}

static bool check_sombra(const Vec3& point) {
    Vec3 to_light = minhaLuz.pos - point;
    float dist    = to_light.length();
    Ray   shadow  = { point, to_light.normalize() };
    HitRecord rec = closest_hit(shadow, T_MIN, dist);
    return rec.hit;
}

static Vec3 trace_raio(const Ray& ray, int depth);

static Vec3 shade(const Ray& ray, const HitRecord& rec, int depth) {
    Vec3 N     = rec.normal.normalize();
    Vec3 to_l  = (minhaLuz.pos - rec.point).normalize();
    Vec3 to_c  = (ray.origin - rec.point).normalize();

    Vec3 ambient = rec.mat.color * 0.12f;

    float diff_f = std::max(0.f, N.dot(to_l));
    Vec3  diffuse = rec.mat.color * diff_f * 0.75f;

    Vec3  R        = (to_l * -1.f).reflect(N);
    float spec_f   = std::pow(std::max(0.f, to_c.dot(R)), rec.mat.shininess);
    Vec3  specular = minhaLuz.color * spec_f * 0.6f;

    if (check_sombra(rec.point)) {
        diffuse  = {0, 0, 0};
        specular = {0, 0, 0};
    }

    Vec3 local = ambient + diffuse + specular;

    // reflete recursivo se o material brilhar
    if (rec.mat.reflectivity > 0.01f && depth < MAX_DEPTH) {
        Vec3 ref_dir  = ray.dir.reflect(N);
        Ray  ref_ray  = { rec.point, ref_dir.normalize() };
        Vec3 ref_col  = trace_raio(ref_ray, depth + 1);
        local = local * (1.f - rec.mat.reflectivity) + ref_col * rec.mat.reflectivity;
    }

    return local;
}

static Vec3 trace_raio(const Ray& ray, int depth) {
    if (depth > MAX_DEPTH) return {0, 0, 0};

    HitRecord rec = closest_hit(ray, T_MIN, T_MAX);

    if (rec.hit) return shade(ray, rec, depth);

    // faz degrade pro ceu
    float t = 0.5f * (ray.dir.normalize().y + 1.0f);
    Vec3 sky_top    = {0.35f, 0.55f, 0.9f};
    Vec3 sky_bottom = {1.f, 1.f, 1.f};
    return sky_bottom * (1.f - t) + sky_top * t;
}

static Camera start_cam(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    float vfov   = 60.f * (3.14159265f / 180.f);
    float half_h = std::tan(vfov / 2.f);
    float half_w = aspect * half_h;

    Camera cam;
    cam.origin           = {0, 0, 1};
    cam.lower_left_corner = {-half_w, -half_h, 0};
    cam.horizontal       = {2.f * half_w, 0, 0};
    cam.vertical         = {0, 2.f * half_h, 0};
    return cam;
}

static Ray cria_raio_cam(const Camera& cam, float u, float v) {
    Vec3 dir = cam.lower_left_corner
             + cam.horizontal * u
             + cam.vertical   * v
             - cam.origin;
    return { cam.origin, dir.normalize() };
}

extern "C" DLL_EXPORT void render(int width, int height, int samples, uint8_t* out_rgb) {
    build_scene();
    Camera cam = start_cam(width, height);

    for (int j = height - 1; j >= 0; j--) {
        for (int i = 0; i < width; i++) {
            Vec3 color = {0, 0, 0};

            for (int s = 0; s < samples; s++) {
                float u = (i + frand()) / static_cast<float>(width);
                float v = (j + frand()) / static_cast<float>(height);
                Ray   r = cria_raio_cam(cam, u, v);
                Vec3  c = trace_raio(r, 0);
                color   = color + c;
            }

            color = color / static_cast<float>(samples);

            color.x = std::pow(clamp01(color.x), 1.f / 2.2f);
            color.y = std::pow(clamp01(color.y), 1.f / 2.2f);
            color.z = std::pow(clamp01(color.z), 1.f / 2.2f);

            int idx = ((height - 1 - j) * width + i) * 3;
            out_rgb[idx + 0] = static_cast<uint8_t>(color.x * 255.99f);
            out_rgb[idx + 1] = static_cast<uint8_t>(color.y * 255.99f);
            out_rgb[idx + 2] = static_cast<uint8_t>(color.z * 255.99f);
        }
    }
}
