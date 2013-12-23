/* is stack page deallocated? */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define assert(x) if (x) {} else { \
  printf(1, "%s: %d ", __FILE__, __LINE__); \
  printf(1, "assert failed (%s)\n", # x); \
  printf(1, "TEST FAILED\n"); \
  exit(); \
}

int
main(int argc, char *argv[])
{
  int i;
  for(i=0; i < 10000; i++) {
    if(fork() == 0) exit();
    else wait();
  }
  printf(1, "TEST PASSED\n");
  exit();
}
