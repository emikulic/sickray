#pragma once

#include <cmath>
#include <cstdint>
#include <memory>

namespace {
constexpr double sqr(double d) { return d * d; }

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
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }

  vec3& operator*=(double d) {
    x *= d;
    y *= d;
    z *= d;
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

struct Sphere {
 public:
  // Returns the distance along the ray.
  double intersect(const Ray& r) const {
    vec3 ec = r.start - center;
    double a = dot(r.dir, r.dir);
    double b = 2. * dot(r.dir, ec);
    double c = dot(ec, ec) - sqr(radius);
    double det = b * b - 4. * a * c;
    if (det < 0) return -1;
    return (-b - sqrt(det)) / (2. * a);
  }

  // Returns the normal vector at p.
  vec3 normal(const vec3& p) const { return normalize(p - center); }

  vec3 center;
  double radius;
};

struct Ground {
 public:
  // Returns the distance along the ray.
  double intersect(const Ray& r) const {
    return (height - r.start.y) / r.dir.y;
  }

  // Returns the normal vector at p.
  vec3 normal(const vec3& p) const { return vec3{0, 1, 0}; }

  double height;
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
