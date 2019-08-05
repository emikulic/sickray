// Visualize randomness.
#include <unistd.h>
#include <iostream>

#include "image.h"
#include "random.h"
#include "show.h"
#include "time.h"
#include "writepng.h"

namespace {

int kWidth = 600;
int kHeight = 400;
int kSamples = 1;  // per pixel.
const char* opt_outfile = nullptr;  // Don't save.
bool want_display = true;
int runs = 1;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:s:o:b:x")) != -1) {
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
      case 'x':
        want_display = false;
        break;
      default:
        std::cerr << "error parsing cmdline flags\n";
    }
  }
}

Image Render() {
  Image out(kWidth, kHeight);
  for (int r = 0; r < runs; ++r) {
    double* ptr = out.data_.get();
    timespec t0 = Now();
    Random rng0;
    for (int y = 0; y < kHeight; ++y) {
      Random rng = rng0.fork(y);
      for (int x = 0; x < kWidth; ++x) {
        double d = 0;
        for (int s = 0; s < kSamples; ++s) {
          d += rng.rand();
        }
        d /= kSamples;
        ptr[0] = d;
        ptr[1] = d;
        ptr[2] = d;
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
