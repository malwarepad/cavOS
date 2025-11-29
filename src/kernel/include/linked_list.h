#include "util.h"

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct LLheader {
  struct LLheader *next;
  // ...
} __attribute__((packed)) LLheader;

#define LL_SIGNATURE_1 0xa8ae0a9b148db421
#define LL_SIGNATURE_2 0x7c9bfa1f171be6da

typedef struct LLcontrol {
  uint64_t signature1; // LL_SIGNATURE_1
  uint64_t signature2; // LL_SIGNATURE_2

  uint32_t structSize;
  Spinlock LOCK_LL;

  LLheader *firstObject;
} LLcontrol;

void  LinkedListInit(LLcontrol *ll, uint32_t structSize);
void *LinkedListAllocate(LLcontrol *ll, uint32_t structSize);
bool  LinkedListUnregister(LLcontrol *ll, uint32_t structSize,
                           const void *LLtarget);
bool  LinkedListRemove(LLcontrol *ll, uint32_t structSize, void *LLtarget);
void  LinkedListPushFrontUnsafe(LLcontrol *ll, void *LLtarget);
void  LinkedListDestroy(LLcontrol *ll, uint32_t structSize);

void  LinkedListTraverse(LLcontrol *ll, void(callback)(void *data, void *ctx),
                         void      *ctx);
void *LinkedListSearch(LLcontrol *ll, bool(isCorrect)(void *data, void *ctx),
                       void      *ctx);
void *LinkedListSearchPtr(LLcontrol *ll, void *targetPtr);

#endif
