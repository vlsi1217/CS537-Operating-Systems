#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
typedef struct __lock_t {
  uint locked;
  uint guard;
} lock_t;

typedef struct __cond_t{
  uint num;
  uint curr;
  lock_t lock;
} cond_t;

#ifndef NULL
#define NULL (0)
#endif

#endif //_TYPES_H_
