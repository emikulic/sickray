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

  friend vec2 operator*(double d, const vec2& v) { return v * d; }

  // Returns a uniformly distributed random point within the unit circle.
  static vec2 uniform_disc(Random& rng) {
    vec2 v;
    do {
      v = 2 * (vec2{rng.rand(), rng.rand()} - vec2{.5, .5});
    } while ((v.x * v.x + v.y * v.y) > 1.);
    return v;
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
  vec3& operator/=(double d) {
    *this = *this / d;
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

  vec2 xy() const { return vec2{x, y}; }
  vec2 xz() const { return vec2{x, z}; }
  vec2 yz() const { return vec2{y, z}; }

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
};

class Shader {
 public:
  vec3 Shade(Random& rng, const Tracer* t, const Object* obj, const Ray& r,
             double dist, int level) const {
    if (light) {
      return color;
    }
    vec3 out{0, 0, 0};
    vec3 p = r.p(dist);
    vec3 n = obj->Normal(p);

    if (diffuse > 0) {
      // Pick random direction.
      vec3 d;
      double shade;
      do {
        d = vec3{rng.rand(), rng.rand(), rng.rand()} - vec3{.5, .5, .5};
        d = normalize(d);
        shade = dot(n, d);
      } while (shade <= 0);

      // Trace.
      vec3 incoming = t->Trace(rng, Ray{p, d}, level + 1);
      out += color * diffuse * shade * incoming;
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

  Shader& set_color(vec3 c) {
    color = c;
    return *this;
  }
  Shader& set_diffuse(double d) {
    diffuse = d;
    return *this;
  }
  Shader& set_reflection(double d) {
    reflection = d;
    return *this;
  }
  Shader& set_checker(bool b) {
    checker = b;
    return *this;
  }
  Shader& set_light(bool b) {
    light = b;
    return *this;
  }

  vec3 color{1, 1, 1};
  double diffuse = 1.;
  double reflection = 0;
  bool checker = false;
  bool light = false;
};

class Scene {
 public:
  struct Elem {
    Elem(Object* o, const Shader& s) : obj(o), shader(s) {}
    Object* obj;
    Shader shader;
  };

  struct Hit {
    double dist;
    const Elem* elem;  // Miss = nullptr.
  };

  // Takes ownership of object.
  void AddElem(Object* o, const Shader& s) {
    elems_.emplace_back(o, s);
    objs_.emplace_back(o);
  }

  void AddBox(const vec3& xyz1, const vec3& xyz2, const Shader& s) {
    AddElem(new RightPlane(xyz1.x, xyz1.yz(), xyz2.yz()), s);
    AddElem(new LeftPlane(xyz2.x, xyz1.yz(), xyz2.yz()), s);
    AddElem(new TopPlane(xyz1.y, xyz1.xz(), xyz2.xz()), s);
    AddElem(new BtmPlane(xyz2.y, xyz1.xz(), xyz2.xz()), s);
    AddElem(new BackPlane(xyz1.z, xyz1.xy(), xyz2.xy()), s);
    AddElem(new FwdPlane(xyz2.z, xyz1.xy(), xyz2.xy()), s);
  }

  void AddRoom(const vec3& xyz1, const vec3& xyz2, const Shader& s) {
    // A box with normals inverted.
    AddElem(new LeftPlane(xyz1.x, xyz1.yz(), xyz2.yz()), s);
    AddElem(new RightPlane(xyz2.x, xyz1.yz(), xyz2.yz()), s);
    AddElem(new BtmPlane(xyz1.y, xyz1.xz(), xyz2.xz()), s);
    AddElem(new TopPlane(xyz2.y, xyz1.xz(), xyz2.xz()), s);
    AddElem(new FwdPlane(xyz1.z, xyz1.xy(), xyz2.xy()), s);
    AddElem(new BackPlane(xyz2.z, xyz1.xy(), xyz2.xy()), s);
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
  std::vector<std::unique_ptr<Object>> objs_;
};
