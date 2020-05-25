// Visualization of bad randomness when xoshiro is used with insufficient
// mixing.
#include <unistd.h>

#include <iostream>

#include "image.h"
#include "random.h"
#include "show.h"
#include "time.h"
#include "writepng.h"

namespace {

int kWidth = 512;
int kHeight = 512;
const char* opt_outfile = nullptr;  // Don't save.
bool want_display = true;
int runs = 1;

void ProcessOpts(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "w:h:o:b:x")) != -1) {
    switch (c) {
      case 'w':
        kWidth = atoi(optarg);
        break;
      case 'h':
        kHeight = atoi(optarg);
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
    for (int y = 0; y < kHeight; ++y) {
      Random rng;
      // This is the mistake: don't set state like this.
      rng.s[0] = 0;
      rng.s[1] = 0;
      rng.s[2] = 0;
      rng.s[3] = y;
      for (int x = 0; x < kWidth; ++x) {
        double d = rng.rand();
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
