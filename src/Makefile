CCACHE=ccache
CXX=g++
CXXFLAGS=-std=c++17 -O3 -march=native -ffast-math -fno-exceptions\
 --all-warnings -fdiagnostics-color
# While compiling, produce a .d (depends) file.
MKDEP=-MMD -MT "$(<:.cc=.o)"

all: show_test
.PHONY: all

# Automatically find sources.
SRCS := $(wildcard *.cc)
DEPS = $(SRCS:.cc=.d)
OBJS = $(SRCS:.cc=.o)

# Include whatever deps files we've got so far, fail silently on files that
# don't exist yet.
sinclude $(DEPS)

# Build all objects and generate dependencies for them.
$(OBJS): %.o: %.cc
	$(CCACHE) $(CXX) $(CXXFLAGS) $(MKDEP) -c -o $@ $<

show_test: show_test.o show.o
	$(CXX) $(CFLAGS) $^ -lX11 -o $@

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJS) show_test
