#pragma once

// There can only be one window open at a time. All calls
// must come from the same thread.
class GLViewer {
 public:
  GLViewer() = delete;

  // Opens a window. `data` is in 8bpp BGRA format and must outlive the call to
  // Close().
  static void Open(int width, int height, void* data);

  // Closes the window.
  static void Close();

  // Polls for keypress or window close.
  static void Poll();

  // Returns false after the user has closed the window (or hit ESC).
  static bool IsRunning();

  // Updates the contents of the window, after data changed.
  static void Update();
};
