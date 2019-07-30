#pragma once

#include <time.h>
#include <iomanip>
#include <iostream>

namespace {

timespec Now() {
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t;
}

timespec operator-(const timespec& a, const timespec& b) {
  timespec out;
  out.tv_sec = a.tv_sec - b.tv_sec;
  out.tv_nsec = a.tv_nsec - b.tv_nsec;
  if (out.tv_nsec < 0) {
    out.tv_sec -= 1;
    out.tv_nsec += 1000000000;
  }
  return out;
}

std::ostream& operator<<(std::ostream& os, const timespec& t) {
  return os << t.tv_sec << '.' << std::setfill('0') << std::setw(9)
            << t.tv_nsec;
}

}  // namespace
