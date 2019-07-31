#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "random.h"

namespace {

template <typename T>
constexpr T sqr(T d) {
  return d * d;
}

template <typename T>
constexpr T max(T a, T b) {
  return (a > b) ? a : b;
}

template <typename T>
constexpr T fract(T f) {
  return f - floor(f);
}

}  // namespace

struct vec2 {
 public:
  vec2 operator*(double d) const { return vec2{x * d, y * d}; }
  vec2 operator/(double d) const { return vec2{x / d, y / d}; }
  vec2 operator+(const vec2& v) const { return vec2{x + v.x, y + v.y}; }
  vec2 operator-(const vec2& v) const { return vec2{x - v.x, y - v.y}; }
  vec2& operator+=(const vec2& v) {
    *this = *this + v;
    return *this;
  }
  vec2& operator-=(const vec2& v) {
    *this = *this - v;
    return *this;
  }
  vec2& operator*=(double d) {
    *this = *this * d;
    return *this;
  }
  vec2& operator/=(double d) {
    *this = *this / d;
    return *this;
  }

  double x, y;
};

struct vec3 {
 public:
  vec3 operator+(const vec3& v) const {
    return vec3{x + v.x, y + v.y, z + v.z};
  }
  vec3 operator-(const vec3& v) const {
    return vec3{x - v.x, y - v.y, z - v.z};
  }

  // Elementwise.
  vec3 operator*(const vec3& v) const {
    return vec3{x * v.x, y * v.y, z * v.z};
  }

  vec3 operator*(double d) const { return vec3{x * d, y * d, z * d}; }
  vec3 operator/(double d) const { return vec3{x / d, y / d, z / d}; }

  // Unary.
  vec3 operator-() const { return vec3{-x, -y, -z}; }

  vec3& operator+=(const vec3& v) {
    *this = *this + v;
    return *this;
  }
  vec3& operator-=(const vec3& v) {
    *this = *this - v;
    return *this;
  }
  vec3& operator*=(const vec3& v) {
    *this = *this * v;
    return *this;
  }
  vec3& operator*=(double d) {
    *this = *this * d;
    return *this;
  }

  friend vec3 operator*(double d, const vec3& v) { return v * d; }

  friend vec3 cross(const vec3& a, const vec3& b) {
    return vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
  }

  friend double dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  friend double length(const vec3& v) { return sqrt(dot(v, v)); }

  friend vec3 normalize(const vec3& v) { return v / length(v); }

  // The normal vector must be a unit vector.
  friend vec3 reflect(const vec3& i, const vec3& n) {
    vec3 c = n * dot(n, -i);
    vec3 p = c + i;
    return c + p;
  }

  double x, y, z;
};

struct Lookat {
 public:
  Lookat(const vec3& camera, const vec3& look)
      : fwd(normalize(look - camera)),
        right(normalize(cross(fwd, vec3{0, 1, 0}))),
        up(cross(right, fwd)) {}

  const vec3 fwd;
  const vec3 right;
  const vec3 up;
};

struct Ray {
 public:
  vec3 p(double dist) const { return start + dir * dist; }

  vec3 start, dir;
};

class Object {
 public:
  // Returns distance along the ray, or a negative number if there is no
  // intersection.
  virtual double Intersect(const Ray& r) const = 0;

  // Returns the normal vector at intersection point p. Must be a unit vector.
  virtual vec3 Normal(const vec3& p) const = 0;
};

class Sphere : public Object {
 public:
  Sphere(vec3 center, double radius) : center(center), radius(radius) {}

  double Intersect(const Ray& r) const override {
    vec3 ec = r.start - center;
    double a = dot(r.dir, r.dir);
    double b = 2. * dot(r.dir, ec);
    double c = dot(ec, ec) - sqr(radius);
    double det = b * b - 4. * a * c;
    if (det < 0) return -1;
    return (-b - sqrt(det)) / (2. * a);
  }

  vec3 Normal(const vec3& p) const override { return normalize(p - center); }

  vec3 center;
  double radius;
};

class Ground : public Object {
 public:
  explicit Ground(double height) : height(height) {}

  double Intersect(const Ray& r) const override {
    return (height - r.start.y) / r.dir.y;
  }

  vec3 Normal(const vec3& p) const override { return vec3{0, 1, 0}; }

  double height;
};

class Tracer {
 public:
  // Returns a color.
  virtual vec3 Trace(Random& rng, const Ray& r, int level) const = 0;
};

class Shader {
 public:
  // Returns a color.
  virtual vec3 Shade(Random& rng, const Tracer* t, const Object* obj,
                     const Ray& r, double dist, const vec3& light,
                     int level) const = 0;
};

class Reflective : public Shader {
 public:
  vec3 Shade(Random& rng, const Tracer* t, const Object* obj, const Ray& r,
             double dist, const vec3& light, int level) const override {
    vec3 p = r.p(dist);
    vec3 n = obj->Normal(p);
    double shade = dot(n, normalize(light - p));
    shade = max(shade, 0.);
    constexpr vec3 metal{.6, .7, .8};  // TODO: make this configurable
    vec3 color = metal * shade * .5;

    // Perturb the normal to blur the reflection.
    vec3 n2 =
        n + (vec3{rng.rand(), rng.rand(), rng.rand()} - vec3{.5, .5, .5}) * .03;
    n2 = normalize(n2);
    Ray refray{p, reflect(p - r.start, n2)};
    color += .5 * metal * t->Trace(rng, refray, level + 1);

    return color;
  }
};

class Scene {
 public:
  struct Elem {
    std::unique_ptr<Object> obj;
    std::unique_ptr<Shader> shader;
  };

  struct Hit {
    double dist;
    const Elem* elem;  // Miss = nullptr.
  };

  // Takes ownership of both pointers.
  void AddElem(Object* o, Shader* s) {
    elems_.push_back(
        Elem{std::unique_ptr<Object>(o), std::unique_ptr<Shader>(s)});
  }

  Hit Intersect(const Ray& ray) const {
    Hit h{-1, nullptr};
    for (const auto& e : elems_) {
      double d = e.obj->Intersect(ray);
      if (Before(d, h.dist)) {
        h.dist = d;
        h.elem = &e;
      }
    }
    return h;
  }

 private:
  // Does a hit before b?
  static bool Before(double a, double b) {
    if (a > 0 && b > 0 && a < b) return true;
    if (a > 0 && b < 0) return true;
    return false;
  }

  std::vector<Elem> elems_;
};

class Image {
 public:
  Image(int width, int height)
      : width_(width), height_(height), data_(new vec3[width_ * height_]) {}

  const int width_;
  const int height_;
  std::unique_ptr<vec3[]> data_;
};

namespace {

template <typename T>
constexpr float clip(T f, T min = 0., T max = 1.) {
  return (f < min) ? min : ((f > max) ? max : f);
}

constexpr uint8_t from_float(float linear, float gamma = 2.2) {
  float out = powf(linear, 1. / gamma);
  out = clip(out);
  return static_cast<uint8_t>(out * 255. + .5);
}

// Convert uniform random rectangle [0,1) to uniform random unit circle.
vec2 uniform_disc(const vec2& v) {
  // http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations.html#SamplingaUnitDisk
  const double r = sqrt(v.x);
  const double a = 2 * M_PI * v.y;
  return vec2{r * cos(a), r * sin(a)};
}

}  // namespace
