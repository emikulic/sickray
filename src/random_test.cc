// Prints out random numbers.
// Example usage: ./random_test | ministat
#include "random.h"

#include <cstdio>

int main() {
  Random r;
  for (int i = 0; i < 2000; ++i) {
    Random copy = r;
    printf("%.52f # %016llx\n", r.rand(), copy.next());
  }
}
