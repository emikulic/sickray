CCACHE=ccache
CXX=g++
CXXFLAGS=-g1 -std=c++17 -O3 -march=native -ffast-math -fno-exceptions\
 --all-warnings -fdiagnostics-color -Wno-narrowing -Wno-sign-compare
# While compiling, produce a .d (depends) file.
MKDEP=-MMD -MT "$(<:.cc=.o) $(<:.cc=.s)"

all: sickray disc_test glviewer_test random_test random_vis show_test
.PHONY: all

# Automatically find sources.
SRCS := $(wildcard *.cc)
DEPS = $(SRCS:.cc=.d)
OBJS = $(SRCS:.cc=.o)
ASMS = $(SRCS:.cc=.s)

# Include whatever deps files we've got so far, fail silently on files that
# don't exist yet.
sinclude $(DEPS)

# Build all objects and generate dependencies for them.
$(OBJS): %.o: %.cc
	$(CCACHE) $(CXX) $(CXXFLAGS) $(MKDEP) -c -o $@ $<

# When requested, produce assembler.
$(ASMS): %.s: %.cc
	$(CXX) $(CXXFLAGS) $(MKDEP) -g0 -fno-asynchronous-unwind-tables \
		-masm=intel -S -o $@ $<

sickray: sickray.o show.o writepng.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -lX11 -pthread -o $@

disc_test: disc_test.o show.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -lX11 -o $@

glviewer_test: glviewer_test.o glviewer.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -lX11 -lGL -pthread -o $@

random_test: random_test.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -o $@

random_vis: random_vis.o show.o writepng.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -lX11 -o $@

show_test: show_test.o show.o
	$(CCACHE) $(CXX) $(CFLAGS) $^ -lX11 -o $@

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJS) $(ASMS) sickray disc_test random_test \
		random_vis show_test
