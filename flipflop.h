#ifndef FLIPFLOP_H
#define FLIPFLOP_H

void failed_ensure(const char *desc, int line);

// Like assert() but always runs, desc is printed if it fails
#define ENSURE(expr, desc) \
  ((expr) \
   ? (void)(0) \
   : failed_ensure(desc, __LINE__));

#endif

