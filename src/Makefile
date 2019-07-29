CXX=g++
CFLAGS=-O3 -march=native -fno-exceptions -Wall

all: show_test

.PHONY: all

show_test: show_test.o show.o
	$(CXX) $(CFLAGS) $^ -lX11 -o $@

show.o: show.cc show.h

show_test.o: show_test.cc show.h

.cc.o:
	$(CXX) $(CFLAGS) -c $< -o $@
