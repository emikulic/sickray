// Renders a sphere.
// Right-handed coordinates.
#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
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
int num_threads = 8;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:s:o:b:l:t:x")) != -1) {
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
      case 't':
        num_threads = atoi(optarg);
        break;
      case 'x':
        want_display = false;
        break;
      default:
        std::cerr << "error parsing cmdline flags\n";
    }
  }
}

std::atomic<bool> running = true;

void sigint_handler(int) { running.store(false, std::memory_order_relaxed); }

// constexpr vec3 kCamera{-2.5, 1, 0};
constexpr vec3 kCamera{-1, 1, 2};
constexpr vec3 kLookAt{0, 1, 0};
constexpr vec3 kFocus{0, 1, 0};
constexpr double kAperture = 1. / 128;  // Amount of focal blur.

class MyScene : public Scene {
 public:
  // Set up scene.
  MyScene() : Scene(kMaxLevel) {
    Shader wall = Shader().set_color({.9, .9, .9});  //.set_checker(true);
    AddRoom({-3, 0, -3}, {3, 2, 3}, wall);

    Shader light = Shader().set_light(true);
    // AddElem(new TopPlane(1.98, {-.2, -.9}, {1.6, .9}), light);

    // Lights on the RHS.
    for (double z = -2.5; z < 3; ++z) {
      AddElem(new RightPlane(2.98, {0.1, z + .1}, {1.5, z + .4}), light);
    }

    // Some pillars.
    Shader pillar = Shader().set_color({.9, .9, .8});
    AddBox({-3, 0, -3}, {-2, 2, -2}, pillar);

    // Pillars on the RHS.
    for (double z = -3; z <= 3; ++z) {
      AddBox({2.5, 0, z}, {3, 2, z + .5}, pillar);
    }

    // AddBox({2, 0, -1}, {3, 2, 0}, pillar);

    // Still life.
    AddBox({-.7, 0, 0}, {-.2, 0.5, .5}, Shader().set_color({1, 0, 0}));
    if (0)
      AddElem(
          new Sphere({1., .5, .5}, .5),
          Shader().set_diffuse(.2).set_reflection(.8).set_color({.7, .8, .9}));
  }
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

void RendererThread(std::atomic<int>* line, const Lookat& look_at,
                    const MyScene& scene, const Random& rng, Image* out) {
  while (1) {
    const int y = line->fetch_add(1, std::memory_order_acq_rel);
    if (y >= kHeight) return;
    double* ptr = out->data_.get() + y * out->width_ * 3;
    Random rngy = rng.fork(y);
    for (int x = 0; x < kWidth; ++x) {
      // rngy.next();
      Random rngx = rngy.fork(x);
      vec3 color{0, 0, 0};
      for (int s = 0; s < kSamples; ++s) {
        // rngx.next();
        Random rng = rngx.fork(s);
        color += RenderPixel(&scene, rng, look_at, vec2{x, y});
      }
      color /= kSamples;
      ptr[0] = color.x;
      ptr[1] = color.y;
      ptr[2] = color.z;
      ptr += 3;
      if (!running.load(std::memory_order_relaxed)) return;
    }
  }
}

Image Render() {
  Image out(kWidth, kHeight);
  const Lookat look_at(kCamera, kLookAt);
  const MyScene scene;
  const Random rng;
  for (int r = 0; r < runs; ++r) {
    std::atomic<int> line = 0;
    timespec t0 = Now();
    {
      // Fork-join.
      std::vector<std::thread> thr;
      thr.reserve(num_threads);
      for (int t = 0; t < num_threads; ++t) {
        thr.emplace_back([&line, &look_at, &scene, &rng, &out]() {
          RendererThread(&line, look_at, scene, rng, &out);
        });
      }
      for (int t = 0; t < num_threads; ++t) {
        thr[t].join();
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
  signal(SIGINT, sigint_handler);
  Image img = Render();
  if (opt_outfile != nullptr) {
    Writepng(img, opt_outfile);
  }
  if (want_display) {
    Show(img);
  }
}
