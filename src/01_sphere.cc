// Renders a sphere.
// Right-handed coordinates.
#include <iostream>

#include "random.h"
#include "ray.h"
#include "show.h"

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr int kSamples = 8;  // per pixel.

constexpr vec3 kCamera{0, .8, 2};
constexpr vec3 kLookAt{.5, 1, 0};
constexpr sphere kSphere{vec3{0, 1, 0}, 1.};
constexpr ground kGround{0};
constexpr vec3 kLightPos{5, 5, 5};

// Does a hit before b?
bool Before(double a, double b) {
  if (a > 0 && b > 0 && a < b) return true;
  if (a > 0 && b < 0) return true;
  return false;
}

vec3 ShadeSky(const ray& r) {
  return vec3{.1, .2, .3} + r.dir.y * vec3{.2, .2, .2};
}

vec3 ShadeSphere(const ray& r, double dist) {
  vec3 p = r.p(dist);
  vec3 n = kSphere.normal(p);
  double shade = dot(n, normalize(kLightPos - p));
  shade = max(shade, 0.);
  constexpr vec3 diffuse{.6, .7, .8};
  constexpr vec3 ambient{.01, .01, .01};
  return ambient + diffuse * shade;
}

vec3 ShadeGround(const ray& r, double dist) {
  vec3 p = r.p(dist);
  vec3 n = kGround.normal(p);
  double shade = dot(n, normalize(kLightPos - p));
  shade = max(shade, 0.);

  // Shadow.
  ray s{p, kLightPos - p};
  const double sd = kSphere.intersect(s);
  if (sd > 0 && sd < 1) shade = 0;  // In shadow.

  bool check = (fract(p.x) < .5) ^ (fract(p.z) < .5);
  vec3 c{.5, .5, .5};
  if (check) c *= .5;
  return c * shade;
}

// Returns color.
vec3 Trace(const ray& r) {
  double sdist = kSphere.intersect(r);
  double gdist = kGround.intersect(r);

  if (sdist < 0 && gdist < 0) return ShadeSky(r);
  if (Before(sdist, gdist)) return ShadeSphere(r, sdist);
  return ShadeGround(r, gdist);
}

// Returns color.
vec3 RenderPixel(Random& rng, vec2 xy) {
  const lookat look(kCamera, kLookAt);

  // Antialiasing: jitter position within pixel.
  xy += vec2{rng.rand(), rng.rand()};
  // Map to [-aspect, +aspect] and [-1, +1].
  xy = (xy - vec2{kWidth, kHeight} / 2.) / (kHeight / 2.);
  // Invert Y.
  xy.y = -xy.y;

  // Camera ray.
  const vec3 dir = look.fwd + look.right * xy.x + look.up * xy.y;
  const ray r{kCamera, dir};
  return Trace(r);
}

image Render() {
  Random rng(0, 0, 0, 1);
  image out(kWidth, kHeight);
  vec3* ptr = out.data_.get();
  timespec t0 = Now();
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      vec3 color{0, 0, 0};
      for (int i = 0; i < kSamples; ++i) {
        color += RenderPixel(rng, vec2{x, y});
      }
      *ptr = color / kSamples;
      ++ptr;
    }
  }
  timespec t1 = Now();
  std::cout << t1 - t0 << " sec\n";
  return out;
}

}  // namespace

int main() {
  image img = Render();
  show(img);
}
