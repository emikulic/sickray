#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>

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
  virtual ~Object() {}

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

class LeftPlane : public Object {
 public:
  LeftPlane(double x, const vec2& yz1, const vec2& yz2)
      : x(x), yz1(yz1), yz2(yz2) {}

  double Intersect(const Ray& r) const override {
    double dist = (x - r.start.x) / r.dir.x;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.y < yz1.x || p.z < yz1.y || p.y > yz2.x || p.z > yz2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{1, 0, 0}; }

  double x;
  vec2 yz1, yz2;
};

class RightPlane : public Object {
 public:
  RightPlane(double x, const vec2& yz1, const vec2& yz2)
      : x(x), yz1(yz1), yz2(yz2) {}

  double Intersect(const Ray& r) const override {
    double dist = (x - r.start.x) / r.dir.x;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.y < yz1.x || p.z < yz1.y || p.y > yz2.x || p.z > yz2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{-1, 0, 0}; }

  double x;
  vec2 yz1, yz2;
};

class FwdPlane : public Object {
 public:
  FwdPlane(double z, const vec2& xy1, const vec2& xy2)
      : z(z), xy1(xy1), xy2(xy2) {}

  double Intersect(const Ray& r) const override {
    double dist = (z - r.start.z) / r.dir.z;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.x < xy1.x || p.y < xy1.y || p.x > xy2.x || p.y > xy2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{0, 0, 1}; }

  double z;
  vec2 xy1, xy2;
};

class BackPlane : public Object {
 public:
  BackPlane(double z, const vec2& xy1, const vec2& xy2)
      : z(z), xy1(xy1), xy2(xy2) {}

  double Intersect(const Ray& r) const override {
    double dist = (z - r.start.z) / r.dir.z;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.x < xy1.x || p.y < xy1.y || p.x > xy2.x || p.y > xy2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{0, 0, -1}; }

  double z;
  vec2 xy1, xy2;
};

class TopPlane : public Object {
 public:
  TopPlane(double y, const vec2& xz1, const vec2& xz2)
      : y(y), xz1(xz1), xz2(xz2) {}

  double Intersect(const Ray& r) const override {
    double dist = (y - r.start.y) / r.dir.y;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.x < xz1.x || p.z < xz1.y || p.x > xz2.x || p.z > xz2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{0, -1, 0}; }

  double y;
  vec2 xz1, xz2;
};

class BtmPlane : public Object {
 public:
  BtmPlane(double y, const vec2& xz1, const vec2& xz2)
      : y(y), xz1(xz1), xz2(xz2) {}

  double Intersect(const Ray& r) const override {
    double dist = (y - r.start.y) / r.dir.y;
    vec3 p = r.p(dist);
    // Is it outside the rectangle?
    if (p.x < xz1.x || p.z < xz1.y || p.x > xz2.x || p.z > xz2.y) return -1;
    return dist;
  }

  vec3 Normal(const vec3& p) const override { return vec3{0, 1, 0}; }

  double y;
  vec2 xz1, xz2;
};

class Tracer {
 public:
  // Returns a color.
  virtual vec3 Trace(Random& rng, const Ray& r, int level) const = 0;

  // Returns distance along the ray to the nearest intersection.
  virtual double IntersectDist(const Ray& ray) const = 0;
};

class Shader {
 public:
  virtual ~Shader() {}

  // Returns a color.
  virtual vec3 Shade(Random& rng, const Tracer* t, const Object* obj,
                     const Ray& r, double dist, const vec3& light,
                     int level) const = 0;
};

class SimpleShader : public Shader {
 public:
  vec3 Shade(Random& rng, const Tracer* t, const Object* obj, const Ray& r,
             double dist, const vec3& light, int level) const override {
    vec3 out = ambient * color;
    vec3 p = r.p(dist);
    vec3 n = obj->Normal(p);

    if (diffuse > 0) {
      // Shadow test.
      Ray shadow{p + 0.001 * n, light - p};
      double dist = t->IntersectDist(shadow);
      if (dist < 0 || dist > 1) {
        double shade = dot(n, normalize(light - p));
        shade = max(shade, 0.);

        if (checker) {
          bool check = (fract(p.x) < .5) ^ (fract(p.z) < .5);
          if (check) shade *= .5;
        }
        out += color * diffuse * shade;
      }
    }

    if (reflection > 0) {
      // Perturb the normal to blur the reflection.
      double amount = 0.03;
      vec3 n2 =
          n + (vec3{rng.rand(), rng.rand(), rng.rand()} - vec3{.5, .5, .5}) *
                  amount;
      n2 = normalize(n2);
      Ray refray{p, reflect(p - r.start, n2)};
      out += color * reflection * t->Trace(rng, refray, level + 1);
    }

    return out;
  }

  SimpleShader* set_color(vec3 c) {
    color = c;
    return this;
  }
  SimpleShader* set_checker(bool b) {
    checker = b;
    return this;
  }
  SimpleShader* set_ambient(double d) {
    ambient = d;
    return this;
  }
  SimpleShader* set_diffuse(double d) {
    diffuse = d;
    return this;
  }
  SimpleShader* set_reflection(double d) {
    reflection = d;
    return this;
  }

  vec3 color{1, 1, 1};
  bool checker = false;
  double ambient = .02;
  double diffuse = 1.;
  double reflection = 0;
};

class Scene {
 public:
  struct Elem {
    Elem(Object* o, Shader* s) : obj(o), shader(s) {}
    Object* obj;
    Shader* shader;
  };

  struct Hit {
    double dist;
    const Elem* elem;  // Miss = nullptr.
  };

  ~Scene() {
    // Free all objects that were added.
    for (const Object* o : objs_) { delete o; }
    for (const Shader* s : shaders_) { delete s; }
    // Zero objects and shaders.
  }

  // Takes ownership of both pointers.
  void AddElem(Object* o, Shader* s) {
    elems_.emplace_back(o, s);
    objs_.insert(o);
    shaders_.insert(s);
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
  std::unordered_set<Object*> objs_;
  std::unordered_set<Shader*> shaders_;
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
