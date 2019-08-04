#pragma once

#include <cmath>
#include <cstdint>
#include <memory>

namespace {

template <typename T>
constexpr float clip(T f, T min = 0., T max = 1.) {
  return (f < min) ? min : ((f > max) ? max : f);
}

}  // namespace

class Image {
 public:
  Image(int width, int height)
      : width_(width),
        height_(height),
        data_(new double[width_ * height_ * 3]) {}

  static uint8_t from_float(float linear, float gamma = 2.2) {
    float out = powf(linear, 1. / gamma);
    out = clip(out);
    return static_cast<uint8_t>(out * 255. + .5);
  }

  const int width_;
  const int height_;
  std::unique_ptr<double> data_;
};
