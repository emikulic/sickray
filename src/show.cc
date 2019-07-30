// Copyright (c) 2013 Emil Mikulic <emikulic@gmail.com>
// Show an image using xlib.
#include <cstdio>
#include <memory>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <err.h>

#include "ray.h"
#include "show.h"

namespace {

#define CHECK_NOTNULL(ptr, msg)                                         \
  do {                                                                  \
    if (ptr == nullptr)                                                 \
      errx(1, "%s:%d: expected not null: %s", __FILE__, __LINE__, msg); \
  } while (0)

class Viewer {
 public:
  Viewer(int width, int height, const void* data)
      : width_(width), height_(height), data_(data) {
    InitX11();
    Run();
  }

  ~Viewer() {
    // Close window.
    img_->data = NULL;  // Don't free the caller's data in XDestroyImage().
    XDestroyImage(img_);
    XFreeGC(dpy_, gc_);
    XDestroyWindow(dpy_, win_);
    XCloseDisplay(dpy_);
  }

 private:
  void InitX11() {
    dpy_ = XOpenDisplay(nullptr);
    CHECK_NOTNULL(dpy_, "XOpenDisplay failed");

    int scr = DefaultScreen(dpy_);
    Colormap cm = DefaultColormap(dpy_, scr);
    XColor gray;
    gray.red = gray.green = gray.blue = 47823;  // pow(0.5, 1.0/2.2) * 65535
    gray.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(dpy_, cm, &gray);

    win_ = XCreateSimpleWindow(dpy_, DefaultRootWindow(dpy_), 0,
                               0,  // Position relative to parent.
                               width_, height_,
                               0,            // Border width.
                               0,            // Border color.
                               gray.pixel);  // Background color.

    // Set window type to "utility" so i3wm will make it float.
    {
      Atom atoms[2] = {intern_atom("_NET_WM_WINDOW_TYPE_UTILITY"), None};
      XChangeProperty(dpy_, win_, intern_atom("_NET_WM_WINDOW_TYPE"), XA_ATOM,
                      32, PropModeReplace, (unsigned char*)atoms, 1);
    }

    XSelectInput(
        dpy_, win_,
        StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask);

    {
      XSizeHints* hints = XAllocSizeHints();
      CHECK_NOTNULL(hints, "XAllocSizeHints() failed");
      hints->min_width = width_;
      hints->min_height = height_;
      hints->flags = PMinSize;
      XSetWMSizeHints(dpy_, win_, hints, XA_WM_NORMAL_HINTS);
      XFree(hints);
    }

    XMapWindow(dpy_, win_);
    XSync(dpy_, False);

    // Wait for MapNotify.
    for (;;) {
      XEvent e;
      XNextEvent(dpy_, &e);
      if (e.type == MapNotify) {
        break;
      }
    }

    XGCValues gcvalues;
    gc_ = XCreateGC(dpy_, win_, 0, &gcvalues);
    Visual* visual;
    {
      XVisualInfo visual_info;
      if (XMatchVisualInfo(dpy_, scr, 24, TrueColor, &visual_info) == 0) {
        errx(1, "XMatchVisualInfo() failed");
      }
      visual = visual_info.visual;
    }

    img_ = XCreateImage(dpy_, visual, 24, ZPixmap, 0, (char*)data_, width_,
                        height_, 32, 0);
    CHECK_NOTNULL(img_, "XCreateImage() failed");
    img_->byte_order = LSBFirst;  // BGRA (MSBFirst is ARGB)

    win_width_ = width_;
    win_height_ = height_;
    running_ = true;
  }

  // Processes all outstanding X11 events, returns whether a repaint is needed.
  bool ProcessXEvents() {
    bool repaint = false;
    // Must call XPending() to read (non-blocking) from socket.
    while (XPending(dpy_)) {
      // Read and process pending event.
      XEvent e;
      XNextEvent(dpy_, &e);  // Blocking.
      switch (e.type) {
        case Expose:
          repaint = true;
          break;

        case ConfigureNotify: {
          // Repaint on size change.
          if (e.xconfigure.width == win_width_ &&
              e.xconfigure.height == win_height_) {
            break;  // No change.
          }
          win_width_ = e.xconfigure.width;
          win_height_ = e.xconfigure.height;
          repaint = true;
          break;
        }

        case KeyPress: {
          KeySym ks = XkbKeycodeToKeysym(dpy_, (KeyCode)e.xkey.keycode, 0, 0);
          if (ks == XK_Escape || ks == XK_q) {
            running_ = false;
          }
          break;
        }

        case KeyRelease:
          break;

        default:
          printf("unhandled event, type = %d\n", e.type);
      }
    }
    return repaint;
  }

  void Repaint() {
    int x = (win_width_ - width_) / 2;
    int y = (win_height_ - height_) / 2;
    XPutImage(dpy_, win_, gc_, img_, 0, 0,  // Src upper left.
              x, y,                         // Dest.
              width_, height_);
    XFlush(dpy_);
  }

  void Run() {
    while (running_) {
      bool repaint = ProcessXEvents();

      // Actually do the repaint.
      if (repaint) {
        Repaint();
      }
    }
  }

  Atom intern_atom(const char* atom_name) {
    Atom a = XInternAtom(dpy_, atom_name, False);
    if (a == Atom(None)) errx(1, "XInternAtom(\"%s\") failed", atom_name);
    return a;
  }

  const int width_;
  const int height_;
  const void* data_;  // Not owned.

  // X11 stuff.
  Display* dpy_;
  Window win_;
  GC gc_;
  XImage* img_;
  int win_width_;
  int win_height_;
  bool running_;

  Viewer(const Viewer&) = delete;
};
}

// data is 8bpp BGRA format.
void Show(int width, int height, const void* data) {
  Viewer v(width, height, data);
}

void Show(const Image& img) {
  const int w = img.width_;
  const int h = img.height_;
  std::unique_ptr<uint8_t[]> data(new uint8_t[w * h * 4]);

  const vec3* src = img.data_.get();
  uint8_t* dst = data.get();
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      dst[0] = from_float(src->z);
      dst[1] = from_float(src->y);
      dst[2] = from_float(src->x);
      dst += 4;
      src += 1;
    }

  Show(w, h, data.get());
}
