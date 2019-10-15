#include "glviewer.h"

#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

namespace {
constexpr int width = 640;
constexpr int height = 480;
}  // namespace

int main() {
  uint8_t data[height][width][4];
  memset(data, 0, sizeof(data));

  std::atomic<bool> running = true;
  constexpr int num_threads = 8;
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&data, &running, i, num_threads]() {
      int pos = 0;
      int gray = (i + 1) * 255 / num_threads;
      bool on = true;
      while (running) {
        for (int y = i * height / num_threads;
             y < (i + 1) * height / num_threads; ++y) {
          int v = on ? gray : 0;
          data[y][pos][0] = v;
          data[y][pos][1] = v;
          data[y][pos][2] = v;
        }
        pos++;
        if (pos >= width) {
          pos -= width;
          on = !on;
        }
        usleep(1000);
      }
    });
  }

  GLViewer::Open(width, height, data);
  while (GLViewer::IsRunning()) {
    GLViewer::Poll();
    GLViewer::Update();
  }
  GLViewer::Close();

  running = false;
  for (int i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
}
