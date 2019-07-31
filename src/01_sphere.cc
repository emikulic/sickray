// Renders a sphere.
// Right-handed coordinates.
#include <unistd.h>
#include <iostream>

#include "random.h"
#include "ray.h"
#include "show.h"
#include "time.h"
#include "writepng.h"

namespace {

int kWidth = 800;
int kHeight = 480;
int kSamples = 8;  // per pixel.
int kMaxLevel = 100;
const char* opt_outfile = nullptr;  // Don't save.
bool want_display = true;
enum { kRect, kCirc } focal_blur = kCirc;
int runs = 1;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:s:o:f:b:x")) != -1) {
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
      case 'b':
        runs = atoi(optarg);
        want_display = false;
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

const Sphere kSphere{vec3{0, 1, 0}, 1.};
const Ground kGround{0};
constexpr vec3 kLightPos{5, 5, 5};

// Does a hit before b?
bool Before(double a, double b) {
  if (a > 0 && b > 0 && a < b) return true;
  if (a > 0 && b < 0) return true;
  return false;
}

vec3 Trace(Random& rng, const Ray& r, int level);

vec3 ShadeSky(const Ray& r) {
  return vec3{.1, .2, .3} + r.dir.y * vec3{.2, .2, .2};
}

vec3 ShadeSphere(Random& rng, const Ray& r, double dist, int level) {
  vec3 p = r.p(dist);
  vec3 n = kSphere.Normal(p);
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
    Ray refray{p, reflect(p - r.start, n2)};
    color += .5 * metal * Trace(rng, refray, level + 1);
  }

  return color;
}

vec3 ShadeGround(const Ray& r, double dist) {
  vec3 p = r.p(dist);
  vec3 n = kGround.Normal(p);
  double shade = dot(n, normalize(kLightPos - p));
  shade = max(shade, 0.) * .9;

  // Shadow.
  Ray s{p, kLightPos - p};
  const double sd = kSphere.Intersect(s);
  if (sd > 0 && sd < 1) shade = 0;  // In shadow.
  // Ambient.
  shade += .02;

  bool check = (fract(p.x) < .5) ^ (fract(p.z) < .5);
  vec3 c{.5, .5, .5};
  if (check) c *= .5;
  return c * shade;
}

// Returns color.
vec3 Trace(Random& rng, const Ray& r, int level) {
  double sdist = kSphere.Intersect(r);
  double gdist = kGround.Intersect(r);

  if (sdist < 0 && gdist < 0) return ShadeSky(r);
  if (Before(sdist, gdist)) return ShadeSphere(rng, r, sdist, level);
  return ShadeGround(r, gdist);
}

// Returns color.
vec3 RenderPixel(Random& rng, const Lookat& look_at, vec2 xy) {
  // Antialiasing: jitter position within pixel.
  xy += vec2{rng.rand(), rng.rand()};
  // Map to [-aspect, +aspect] and [-1, +1].
  xy = (xy - vec2{kWidth, kHeight} / 2.) / (kHeight / 2.);
  // Invert Y.
  xy.y = -xy.y;

  // Point on projection plane.
  double focal_dist = length(kFocus - kCamera);
  vec3 dir = look_at.fwd + look_at.right * xy.x + look_at.up * xy.y;
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
  vec3 camera = kCamera + (look_at.right * blur.x) + (look_at.up * blur.y);
  return Trace(rng, Ray{camera, proj - camera}, /*level=*/0);
}

Image Render() {
  Image out(kWidth, kHeight);
  const Lookat look_at(kCamera, kLookAt);
  for (int r = 0; r < runs; ++r) {
    Random rng(0, 0, 0, 1);
    vec3* ptr = out.data_.get();
    timespec t0 = Now();
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        vec3 color{0, 0, 0};
        for (int s = 0; s < kSamples; ++s) {
          color += RenderPixel(rng, look_at, vec2{x, y});
        }
        *ptr = color / kSamples;
        ++ptr;
      }
    }
    timespec t1 = Now();
    std::cout << t1 - t0 << " sec" << std::endl;  // Flush.
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  ProcessOpts(argc, argv);
  Image img = Render();
  if (opt_outfile != nullptr) {
    Writepng(img, opt_outfile);
  }
  if (want_display) {
    Show(img);
  }
}
