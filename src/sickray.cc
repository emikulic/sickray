// Renders a sphere.
// Right-handed coordinates.
#include <unistd.h>
#include <iostream>
#include <memory>
#include <vector>

#include "image.h"
#include "random.h"
#include "ray.h"
#include "show.h"
#include "time.h"
#include "writepng.h"

namespace {

int kWidth = 600;
int kHeight = 400;
int kSamples = 4;  // per pixel.
int kMaxLevel = 2;
const char* opt_outfile = nullptr;  // Don't save.
bool want_display = true;
int runs = 1;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:s:o:b:l:x")) != -1) {
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
      case 'b':
        runs = atoi(optarg);
        want_display = false;
        break;
      case 'l':
        kMaxLevel = atoi(optarg);
        break;
      case 'x':
        want_display = false;
        break;
      default:
        std::cerr << "error parsing cmdline flags\n";
    }
  }
}

constexpr vec3 kCamera{0, 1, 2};
constexpr vec3 kLookAt{0, 1, 0};
constexpr vec3 kFocus{0, 1, 0};
constexpr double kAperture = 1. / 128;  // Amount of focal blur.

class MyTracer : public Tracer {
 public:
  // Set up scene.
  MyTracer() {
    Shader wall = Shader().set_color({.9, .9, .9});  //.set_checker(true);
    scene_.AddRoom({-3, 0, -3}, {3, 2, 3}, wall);

    scene_.AddElem(new TopPlane(1.98, {-.2, -.2}, {1.6, .2}),
                   Shader().set_light(true));

    // Some pillars.
    Shader pillar = Shader().set_color({.9, .9, .8});
    scene_.AddBox({-3, 0, -3}, {-2, 2, -2}, pillar);
    scene_.AddBox({2.5, 0, -3}, {3, 2, -2.5}, pillar);
    scene_.AddBox({2, 0, -1}, {3, 2, 0}, pillar);

    // Still life.
    scene_.AddBox({-1.5, 0, 0}, {-1., 0.5, .5}, Shader().set_color({1, 0, 0}));
    scene_.AddElem(
        new Sphere({1., .5, .5}, .5),
        Shader().set_diffuse(.2).set_reflection(.8).set_color({.7, .8, .9}));
  }

  MyTracer(const MyTracer&) = delete;

  // Returns color.
  vec3 Trace(Random& rng, const Ray& r, int level) const override {
    if (level > kMaxLevel) {
      // Terminate recursion.
      return vec3{0, 0, 0};
    }
    Scene::Hit h = scene_.Intersect(r);
    if (h.elem == nullptr) {
      return {0, 0, 0};
    }
    return h.elem->shader.Shade(rng, this, h.elem->obj, r, h.dist, level);
  }

  Scene scene_;
};

// Returns color.
vec3 RenderPixel(const Tracer* t, Random& rng, const Lookat& look_at, vec2 xy) {
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
  vec2 blur = vec2::uniform_disc(rng) * kAperture;
  vec3 camera = kCamera + (look_at.right * blur.x) + (look_at.up * blur.y);
  return t->Trace(rng, Ray{camera, proj - camera}, /*level=*/0);
}

Image Render() {
  Image out(kWidth, kHeight);
  const Lookat look_at(kCamera, kLookAt);
  MyTracer t;
  for (int r = 0; r < runs; ++r) {
    Random rng(0, 0, 0, 1);
    double* ptr = out.data_.get();
    timespec t0 = Now();
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        vec3 color{0, 0, 0};
        for (int s = 0; s < kSamples; ++s) {
          color += RenderPixel(&t, rng, look_at, vec2{x, y});
        }
        color /= kSamples;
        ptr[0] = color.x;
        ptr[1] = color.y;
        ptr[2] = color.z;
        ptr += 3;
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
