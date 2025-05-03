#include "util.h"

#ifndef AVL_TREE_H
#define AVL_TREE_H

typedef uint64_t avlkey;
typedef uint64_t avlval;

typedef struct __attribute__((packed)) AVLheader {
  avlkey            key;
  struct AVLheader *left;
  struct AVLheader *right;
  int               height;
  avlval            value;
  // ...
} AVLheader;

// global thing for all BSTs
avlval AVLLookup(void *root, avlkey key);

void *AVLAllocate(void **AVLfirstPtr, avlkey key, avlval value);
bool  AVLUnregister(void **AVLfirstPtr, avlkey key);
bool  AVLFree(void **AVLfirstPtr, avlkey key);

#endif
