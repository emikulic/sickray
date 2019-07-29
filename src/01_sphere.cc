// Renders a sphere.
// Right-handed coordinates.
#include <iostream>

#include "ray.h"
#include "show.h"

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;

constexpr vec3 kCamera{0, 2, 3};
constexpr vec3 kLookAt{0, 1, 0};
constexpr sphere kSphere{vec3{0, 1, 0}, 1.};
constexpr vec3 kLightPos{5, 5, 5};

// Returns color.
vec3 Trace(const ray& r) {
  vec3 out{.1, .2, .3};

  double dist = kSphere.intersect(r);
  if (dist < 0) return out;  // Miss.
  vec3 p = r.p(dist);
  vec3 n = kSphere.normal(p);

  double shade = dot(n, normalize(kLightPos - p));
  if (shade < 0) shade = 0;
  return vec3{.6, .7, .8} * shade;
}

// Returns color.
vec3 RenderPixel(vec2 xy) {
  // LookAt vectors.
  const vec3 fwd = normalize(kLookAt - kCamera);
  const vec3 right = normalize(cross(fwd, vec3{0, 1, 0}));
  const vec3 up = cross(right, fwd);  // Unit length.

  // Ray through center of pixel.
  xy += vec2{.5, .5};
  // Map to [-aspect, +aspect] and [-1, +1].
  xy = (xy - vec2{kWidth, kHeight} / 2.) / (kHeight / 2.);
  // Invert Y.
  xy.y = -xy.y;

  // Camera ray.
  const vec3 dir = fwd + right * xy.x + up * xy.y;
  const ray r{kCamera, dir};
  return Trace(r);
  // return vec3{xy.x, xy.y, 1};
}

image Render() {
  image out(kWidth, kHeight);
  vec3* ptr = out.data_.get();
  timespec t0 = Now();
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      *ptr = RenderPixel(vec2{x, y});
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
