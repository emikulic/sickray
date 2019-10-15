// Copyright (c) 2013, 2015, 2019 Emil Mikulic <emikulic@gmail.com>
// Don't make libGL calls from different threads.
#include "glviewer.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>  // apt-get install mesa-common-dev
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <err.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

#define CHECK(cond)              \
  do {                           \
    if (!(cond)) errx(1, #cond); \
  } while (0)

extern "C" {
int ignore_x11_error(Display* d __attribute__((__unused__)),
                     XErrorEvent* e __attribute__((__unused__))) {
  return 0;
}
}

class GLWindow {
 public:
  GLWindow(int width, int height, void* data);
  ~GLWindow();

  bool IsRunning() const { return running_; }
  void EventPoll();
  void Update();

 private:
  // Init and done split up into phases.
  void InitX11();
  void InitGLX();
  void InitGL();
  void DoneGL();
  void DoneX11();

  // X11 and GLX helpers.
  Atom InternAtom(const char* atom_name);
  bool HasGLXExtension(const char* ext);
  __GLXextFuncPtr GetGLXProc(const char* name);
  GLXContext CreateGLXContext(int major, int minor, GLXFBConfig fbconfig);
  void SetSwapInterval(int interval);

  // GL helpers.
  static GLenum MakeShader(const char* src, GLenum shaderType);
  static GLenum MakeProgram(const char* vertex_src, const char* frag_src);
  static GLuint GetAttribLocationOrDie(GLenum program, const char* name);
  static GLuint MakeArrayBuffer(GLuint attrib_location, int len,
                                const double* data, int dimensions);

  int width_;
  int height_;
  void* data_;  // Not owned.

  Display* dpy_ = nullptr;
  Window win_ = None;
  int scr_;
  std::string glx_exts_;
  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;
  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = nullptr;
  PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = nullptr;
  GLuint buf_xyz_;
  GLuint buf_uv_;
  GLuint buf_index_;
  GLenum my_program_;

  bool running_ = true;
};

GLWindow::GLWindow(int width, int height, void* data)
    : width_(width), height_(height), data_(data) {
  InitX11();
  InitGLX();
  InitGL();
}

GLWindow::~GLWindow() {
  DoneGL();
  DoneX11();
}

void GLWindow::EventPoll() {
  // Don't want read to block.
  if (!XPending(dpy_)) return;
  // Read and process pending event.
  XEvent e;
  XNextEvent(dpy_, &e);  // Blocking.
  switch (e.type) {
    case KeyPress: {
      KeySym ks = XkbKeycodeToKeysym(dpy_, (KeyCode)e.xkey.keycode, 0, 0);
      if (ks == XK_Escape || ks == XK_q) {
        running_ = false;
      }
      break;
    }
  }
}

void GLWindow::Update() {
  GLint level = 0;
  GLint border = 0;  // Must be zero according to manpage.
  GLint internalFormat = GL_RGB;
  // nvidia recommends GL_BGRA format in:
  // ftp://download.nvidia.com/developer/Papers/2005/Fast_Texture_Transfers/Fast_Texture_Transfers.pdf
  GLenum format = GL_BGRA;
  glTexImage2D(GL_TEXTURE_2D, level, internalFormat, width_, height_, border,
               format, GL_UNSIGNED_BYTE, data_);
  glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, 0);
  // glFlush() isn't necessary.
  // Don't glFinish() - it's not needed and it spins on CPU.
  glXSwapBuffers(dpy_, win_);  // This blocks until VSync!
}

void GLWindow::InitX11() {
  // We don't need to XInitThreads() because we don't call into xlib
  // concurrently.
  dpy_ = XOpenDisplay(nullptr);
  if (!dpy_) errx(1, "XOpenDisplay failed");
  scr_ = DefaultScreen(dpy_);
  unsigned long bg_color = BlackPixel(dpy_, scr_);
  win_ = XCreateSimpleWindow(dpy_, DefaultRootWindow(dpy_), 0,
                             0,  // Position relative to parent.
                             width_, height_,
                             0,  // Border width.
                             0,  // Border color.
                             bg_color);

  // Set window type to "utility" so i3wm will make it float.
  {
    Atom atoms[2] = {InternAtom("_NET_WM_WINDOW_TYPE_UTILITY"), None};
    XChangeProperty(dpy_, win_, InternAtom("_NET_WM_WINDOW_TYPE"), XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)atoms, 1);
  }

  XStoreName(dpy_, win_, "Hit ESC to close");  // Title.

  XSelectInput(dpy_, win_, StructureNotifyMask | KeyPressMask);

  {
    XSizeHints* hints = XAllocSizeHints();
    if (!hints) errx(1, "XAllocSizeHints() failed");
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
}

void GLWindow::InitGLX() {
  // Function pointers.
  if (HasGLXExtension("GLX_EXT_swap_control")) {
    glXSwapIntervalEXT =
        PFNGLXSWAPINTERVALEXTPROC(GetGLXProc("glXSwapIntervalEXT"));
  } else if (HasGLXExtension("GLX_SGI_swap_control")) {
    glXSwapIntervalSGI =
        PFNGLXSWAPINTERVALSGIPROC(GetGLXProc("glXSwapIntervalSGI"));
  } else {
    CHECK(false &&
          "Can't find either GLX_EXT_swap_control or GLX_SGI_swap_control");
  }
  CHECK(HasGLXExtension("GLX_ARB_create_context"));
  CHECK(HasGLXExtension("GLX_ARB_create_context_profile"));
  glXCreateContextAttribsARB = PFNGLXCREATECONTEXTATTRIBSARBPROC(
      GetGLXProc("glXCreateContextAttribsARB"));

  CHECK(HasGLXExtension("GLX_EXT_create_context_es2_profile"));

  GLXFBConfig* native_configs;
  int native_count;
  native_configs = glXGetFBConfigs(dpy_, scr_, &native_count);
  CHECK(native_configs);
  CHECK(native_count > 0);

  // from:
  // http://www.opengl.org/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
  static int visual_attribs[] = {GLX_X_RENDERABLE,
                                 True,
                                 GLX_DRAWABLE_TYPE,
                                 GLX_WINDOW_BIT,
                                 GLX_RENDER_TYPE,
                                 GLX_RGBA_BIT,
                                 GLX_X_VISUAL_TYPE,
                                 GLX_TRUE_COLOR,
                                 GLX_RED_SIZE,
                                 8,
                                 GLX_GREEN_SIZE,
                                 8,
                                 GLX_BLUE_SIZE,
                                 8,
                                 GLX_STENCIL_SIZE,
                                 0,
                                 GLX_DOUBLEBUFFER,
                                 True,
                                 None};

  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(dpy_, scr_, visual_attribs, &fbcount);
  CHECK(fbc);

  // With the nvidia driver, native_configs comes out sorted such that the
  // first element is the optimal choice. Intel not so.
  GLXFBConfig fbconfig = fbc[0];

  GLXContext glxctx = CreateGLXContext(3, 3, fbconfig);
  CHECK(glxctx);
  CHECK(glXMakeCurrent(dpy_, win_, glxctx) == True);
  CHECK(glXIsDirect(dpy_, glxctx) == True);
  XFree(fbc);
  XFree(native_configs);

  SetSwapInterval(1);
}

#define STRING(x) #x
#define XSTRING(x) STRING(x)

void GLWindow::InitGL() {
  // Vertex shader is the identity function for gl_Position and texture coords.
  static const char *vertex_src = "\
#version 330 core\n\
#line " XSTRING(__LINE__) "\n\
  in vec3 xyz;\n\
  in vec2 uv;\n\
  "
  // Output data is interpolated for every fragment.
  "\
  out vec2 out_uv;\n\
  void main() {\n\
    gl_Position = vec4(xyz, 1);\n\
    out_uv = uv;\n\
  }\n\
  ";

  // Fragment shader uses texture coords to look up into the texture.
  static const char *frag_src = "\
#version 330 core\n\
#line " XSTRING(__LINE__) "\n\
  precision mediump float;\n\
  out vec4 fragColor;\n\
  in vec2 out_uv;\n\
  uniform sampler2D myTextureSampler;\n\
  void main(void) {\n\
    fragColor = texture(myTextureSampler, out_uv).rgba;\n\
  }\n\
  ";

  my_program_ = MakeProgram(vertex_src, frag_src);
  GLuint xyz_slot = GetAttribLocationOrDie(my_program_, "xyz");
  GLuint uv_slot = GetAttribLocationOrDie(my_program_, "uv");

  // Array of vertices.
  {
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
  }

  // XYZ coords of vertices.
  //
  //  1<--0
  //  |
  //  v
  //  2-->3
  double pos[] = {1, 1, 0, -1, 1, 0, -1, -1, 0, 1, -1, 0};
  buf_xyz_ = MakeArrayBuffer(xyz_slot, sizeof(pos), pos, 3);

  // UV coords.
  // double uv_y_is_up[] = {1, 1, 0, 1, 0, 0, 1, 0};
  double uv_y_is_down[] = {1, 0, 0, 0, 0, 1, 1, 1};
  buf_uv_ = MakeArrayBuffer(uv_slot, sizeof(uv_y_is_down), uv_y_is_down, 2);

  // Index into vertices.
  GLuint index[] = {0, 1, 2, 2, 3, 0};
  glGenBuffers(1, &buf_index_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_index_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

  {
    GLuint TextureID;
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }

  glDisable(GL_DEPTH_TEST);
  glUseProgram(my_program_);
}

void GLWindow::DoneGL() {
  glDeleteProgram(my_program_);
  glDeleteBuffers(1, &buf_xyz_);
  glDeleteBuffers(1, &buf_uv_);
  glDeleteBuffers(1, &buf_index_);
}

void GLWindow::DoneX11() {
  XDestroyWindow(dpy_, win_);
  XCloseDisplay(dpy_);
}

Atom GLWindow::InternAtom(const char* atom_name) {
  Atom a = XInternAtom(dpy_, atom_name, False);
  if (a == Atom(None)) errx(1, "XInternAtom(\"%s\") failed", atom_name);
  return a;
}

bool GLWindow::HasGLXExtension(const char* ext) {
  if (glx_exts_.empty()) {
    // Get list of GLX extensions.
    const char* exts = glXQueryExtensionsString(dpy_, scr_);
    CHECK(exts);
    glx_exts_ = " " + std::string(exts) + " ";
  }
  return glx_exts_.find(std::string(ext) + " ") != std::string::npos;
}

__GLXextFuncPtr GLWindow::GetGLXProc(const char* name) {
  __GLXextFuncPtr out = glXGetProcAddressARB((const GLubyte*)name);
  if (!out) errx(1, "glXGetProcAddressARB(\"%s\") failed", name);
  return out;
}

GLXContext GLWindow::CreateGLXContext(int major, int minor,
                                      GLXFBConfig fbconfig) {
  Bool direct = True;
  int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB,
                           major,
                           GLX_CONTEXT_MINOR_VERSION_ARB,
                           minor,
                           GLX_CONTEXT_FLAGS_ARB,
                           GLX_CONTEXT_DEBUG_BIT_ARB,
                           None};
  XSetErrorHandler(ignore_x11_error);
  GLXContext glxctx = glXCreateContextAttribsARB(
      dpy_, fbconfig, nullptr /*share list*/, direct, context_attribs);
  // XSync to force errors to be processed.
  XSync(dpy_, False);
  XSetErrorHandler(nullptr);
  if (!glxctx) {
    return nullptr;
  }
  CHECK(glxctx);
  CHECK(glXMakeCurrent(dpy_, win_, glxctx));
  CHECK(glXIsDirect(dpy_, glxctx) == True);
  return glxctx;
}

void GLWindow::SetSwapInterval(int interval) {
  if (glXSwapIntervalEXT) {
    GLXDrawable drawable = glXGetCurrentDrawable();
    CHECK(drawable);
    glXSwapIntervalEXT(dpy_, drawable, interval);
  } else {
    glXSwapIntervalSGI(interval);
  }
}

// static
GLenum GLWindow::MakeShader(const char* src, GLenum shaderType) {
  GLenum shader = glCreateShader(shaderType);
  if (!shader) errx(1, "glCreateShader() failed");
  glShaderSource(shader, /* count = */ 1, &src, /* string lengths = */ nullptr);
  glCompileShader(shader);
  int val;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    int len;
    glGetShaderInfoLog(shader, sizeof(log), &len, log);
    if (len) {
      const std::string s(log, len);
      fprintf(stderr, "shader info log: %s\n", s.c_str());
    }
  }
  if (val != GL_TRUE) errx(1, "glCompileShader() call failed");
  return shader;
}

// static
GLenum GLWindow::MakeProgram(const char* vertex_src, const char* frag_src) {
  GLenum program = glCreateProgram();
  CHECK(program);
  GLenum vertex_shader = MakeShader(vertex_src, GL_VERTEX_SHADER_ARB);
  glAttachShader(program, vertex_shader);
  GLenum frag_shader = MakeShader(frag_src, GL_FRAGMENT_SHADER_ARB);
  glAttachShader(program, frag_shader);
  glLinkProgram(program);
  char log[4096];
  int len;
  glGetProgramInfoLog(program, sizeof(log), &len, log);
  if (len) {
    const std::string s(log, len);
    fprintf(stderr, "program info log: %s\n", s.c_str());
  }
  int val;
  glGetProgramiv(program, GL_LINK_STATUS, &val);
  if (val != GL_TRUE) errx(1, "GL program link failed");
  glValidateProgram(program);
  glGetProgramiv(program, GL_VALIDATE_STATUS, &val);
  if (val != GL_TRUE) errx(1, "GL program validate failed");
  glDeleteShader(vertex_shader);
  glDeleteShader(frag_shader);
  return program;
}

// static
GLuint GLWindow::GetAttribLocationOrDie(GLenum program, const char* name) {
  GLint out = glGetAttribLocation(program, name);
  if (out == -1) errx(1, "Failed to glGetAttribLocation(\"%s\")", name);
  return GLuint(out);
}

// Allocate and populate a new array buffer, corresponding to the given
// attrib_location. Returns the buffer number.
//
// static
GLuint GLWindow::MakeArrayBuffer(GLuint attrib_location, int len,
                                 const double* data, int dimensions) {
  GLuint buf;
  glGenBuffers(1, &buf);
  glBindBuffer(GL_ARRAY_BUFFER, buf);

  // STATIC = contents will be modified once and used many times.
  // DRAW = contents are modified by the application (not by
  // reading data from GL).
  glBufferData(GL_ARRAY_BUFFER, len, data, GL_STATIC_DRAW);

  // If enabled, the values in the generic vertex attribute array will be
  // accessed and used for rendering when calls are made to vertex array
  // commands such as glDrawArrays.
  glEnableVertexAttribArray(attrib_location);

  GLboolean normalized = GL_FALSE;
  GLsizei stride = 0;  // Tightly packed.
  GLvoid* offset = 0;
  glVertexAttribPointer(attrib_location, dimensions, GL_DOUBLE, normalized,
                        stride, offset);
  return buf;
}

GLWindow* window = nullptr;

}  // namespace

// static
void GLViewer::Open(int width, int height, void* data) {
  CHECK(window == nullptr);
  window = new GLWindow(width, height, data);
}

// static
void GLViewer::Close() {
  delete window;
  window = nullptr;
}

// static
void GLViewer::Poll() { window->EventPoll(); }

// static
bool GLViewer::IsRunning() { return window->IsRunning(); }

// static
void GLViewer::Update() { window->Update(); }
