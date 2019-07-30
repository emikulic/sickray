// Renders a sphere.
// Right-handed coordinates.
#include <unistd.h>
#include <iostream>

#include "random.h"
#include "ray.h"
#include "show.h"
#include "writepng.h"

namespace {

int kWidth = 800;
int kHeight = 480;
int kSamples = 8;  // per pixel.
int kMaxLevel = 100;
const char* opt_outfile = nullptr;  // Don't save.
bool want_display = true;
enum { kRect, kCirc } focal_blur = kCirc;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:s:o:f:x")) != -1) {
    switch (c) {
      case 'w':
        kWidth = atoi(optarg);
        break;
      case 'h':
        kHeight = atoi(optarg);
        break;
      case 's':
        kSamples = atoi(optarg);
        break;
      case 'o':
        opt_outfile = optarg;
        break;
      case 'f':
        if (strcmp(optarg, "circ") == 0) {
          focal_blur = kCirc;
        } else if (strcmp(optarg, "rect") == 0) {
          focal_blur = kRect;
        } else {
          std::cerr << "unknown focal blur type \"" << optarg << "\"\n";
        }
        break;
      case 'x':
        want_display = false;
        break;
      default:
        std::cerr << "error parsing cmdline flags\n";
    }
  }
}

constexpr vec3 kCamera{0, .8, 2};
constexpr vec3 kLookAt{.5, 1, 0};
constexpr vec3 kFocus{0, 0, .5};
constexpr double kAperture = 1. / 24;  // Amount of focal blur.

constexpr sphere kSphere{vec3{0, 1, 0}, 1.};
constexpr ground kGround{0};
constexpr vec3 kLightPos{5, 5, 5};

// Does a hit before b?
bool Before(double a, double b) {
  if (a > 0 && b > 0 && a < b) return true;
  if (a > 0 && b < 0) return true;
  return false;
}

vec3 Trace(Random& rng, const ray& r, int level);

vec3 ShadeSky(const ray& r) {
  return vec3{.1, .2, .3} + r.dir.y * vec3{.2, .2, .2};
}

vec3 ShadeSphere(Random& rng, const ray& r, double dist, int level) {
  vec3 p = r.p(dist);
  vec3 n = kSphere.normal(p);
  double shade = dot(n, normalize(kLightPos - p));
  shade = max(shade, 0.);
  constexpr vec3 metal{.6, .7, .8};
  vec3 color = metal * shade * .5;

  // Reflection.
  if (level < kMaxLevel) {
    // Perturb the normal to blur the reflection.
    vec3 n2 =
        n + (vec3{rng.rand(), rng.rand(), rng.rand()} - vec3{.5, .5, .5}) * .03;
    n2 = normalize(n2);
    ray refray{p, reflect(p - r.start, n2)};
    color += .5 * metal * Trace(rng, refray, level + 1);
  }

  return color;
}

vec3 ShadeGround(const ray& r, double dist) {
  vec3 p = r.p(dist);
  vec3 n = kGround.normal(p);
  double shade = dot(n, normalize(kLightPos - p));
  shade = max(shade, 0.) * .9;

  // Shadow.
  ray s{p, kLightPos - p};
  const double sd = kSphere.intersect(s);
  if (sd > 0 && sd < 1) shade = 0;  // In shadow.
  // Ambient.
  shade += .02;

  bool check = (fract(p.x) < .5) ^ (fract(p.z) < .5);
  vec3 c{.5, .5, .5};
  if (check) c *= .5;
  return c * shade;
}

// Returns color.
vec3 Trace(Random& rng, const ray& r, int level) {
  double sdist = kSphere.intersect(r);
  double gdist = kGround.intersect(r);

  if (sdist < 0 && gdist < 0) return ShadeSky(r);
  if (Before(sdist, gdist)) return ShadeSphere(rng, r, sdist, level);
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

  // Point on projection plane.
  double focal_dist = length(kFocus - kCamera);
  vec3 dir = look.fwd + look.right * xy.x + look.up * xy.y;
  vec3 proj = kCamera + focal_dist * normalize(dir);

  // Focal blur: jitter camera position.
  vec2 blur{rng.rand(), rng.rand()};
  switch (focal_blur) {
    case kCirc:
      blur = uniform_disc(blur);
      break;
    case kRect:
      blur -= vec2{.5, .5};
      blur *= 2.;
      break;
  };
  blur *= kAperture;
  vec3 camera = kCamera + (look.right * blur.x) + (look.up * blur.y);
  return Trace(rng, ray{camera, proj - camera}, /*level=*/0);
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

int main(int argc, char** argv) {
  ProcessOpts(argc, argv);
  image img = Render();
  if (opt_outfile != nullptr) {
    writepng(img, opt_outfile);
  }
  if (want_display) {
    show(img);
  }
}
