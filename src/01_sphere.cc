// Renders a sphere.
// Right-handed coordinates.
#include <unistd.h>
#include <iostream>
#include <memory>
#include <vector>

#include "random.h"
#include "ray.h"
#include "show.h"
#include "time.h"
#include "writepng.h"

namespace {

int kWidth = 800;
int kHeight = 400;
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

constexpr vec3 kCamera{0, .8, 2.5};
constexpr vec3 kLookAt{.5, 1, 0};
constexpr vec3 kFocus{0, 0, .5};
constexpr double kAperture = 1. / 24;  // Amount of focal blur.
constexpr vec3 kLightPos{5, 5, 5};

vec3 ShadeSky(const Ray& r) {
  return vec3{.1, .2, .3} + r.dir.y * vec3{.2, .2, .2};
}

class MyTracer : public Tracer {
 public:
  MyTracer() {
    // Set up scene.
    scene_.AddElem(
        new Ground(0),
        (new SimpleShader)->set_color({.5, .5, .5})->set_checker(true));
    scene_.AddElem(new Sphere({0, 1, 0}, .5), (new SimpleShader)
                                                  ->set_color({.6, .7, .8})
                                                  ->set_reflection(.5)
                                                  ->set_diffuse(.5));
    scene_.AddElem(new LeftPlane(-1, {0, -1}, {1, 1}),
                   (new SimpleShader)->set_color({1, 0, 0}));
    scene_.AddElem(new RightPlane(1, {0, -1}, {1, 1}),
                   (new SimpleShader)->set_color({0, 1, 0}));
    scene_.AddElem(new FwdPlane(-1, {-1, 1}, {1, 3}),
                   (new SimpleShader)->set_color({1, 1, 0}));
    scene_.AddElem(new TopPlane(2, {-1, -1}, {1, 0}),
                   (new SimpleShader)->set_color({0, 1, 1}));
    scene_.AddElem(new BtmPlane(.05, {-.5, -.5}, {.5, .5}),
                   (new SimpleShader)->set_color({1, 0, 1}));
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
      // TODO: replace with sky object.
      return ShadeSky(r);
    }

    return h.elem->shader->Shade(rng, this, h.elem->obj, r, h.dist, kLightPos,
                                 level);
  }

  double IntersectDist(const Ray& ray) const override {
    return scene_.Intersect(ray).dist;
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
  return t->Trace(rng, Ray{camera, proj - camera}, /*level=*/0);
}

Image Render() {
  Image out(kWidth, kHeight);
  const Lookat look_at(kCamera, kLookAt);
  MyTracer t;
  for (int r = 0; r < runs; ++r) {
    Random rng(0, 0, 0, 1);
    vec3* ptr = out.data_.get();
    timespec t0 = Now();
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        vec3 color{0, 0, 0};
        for (int s = 0; s < kSamples; ++s) {
          color += RenderPixel(&t, rng, look_at, vec2{x, y});
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
