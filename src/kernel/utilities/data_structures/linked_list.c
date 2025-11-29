#include <linked_list.h>
#include <malloc.h>
#include <util.h>

// Linked Lists (singly, non-circular); basically just allocated structs
// pointing at each other! Used literally everywhere in the system...
// Copyright (C) 2024 Panagiotis

/*
 * Requirement for this DS utility is that the next entry of the linked list is
 * prepended! Please consider this during usage, otherwise results WILL be
 * fatal!
 *
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
 * | <-    void* next   -> | <-      rest       -> |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

force_inline void LinkedListNormal(LLcontrol *ll, uint32_t structSize) {
  assert(ll->signature1 == LL_SIGNATURE_1);
  assert(ll->signature2 == LL_SIGNATURE_2);
  assert(ll->structSize == structSize);
}

void *LinkedListAllocate(LLcontrol *ll, uint32_t structSize) {
  LinkedListNormal(ll, structSize);
  void **LLfirstPtr = (void **)(&ll->firstObject);

  LLheader *target = (LLheader *)malloc(structSize);
  memset(target, 0, structSize);

  LLheader *curr = (LLheader *)(*LLfirstPtr);
  while (1) {
    if (curr == 0) {
      // means this is our first one
      *LLfirstPtr = target;
      break;
    }
    if (curr->next == 0) {
      // next is non-existent (end of linked list)
      curr->next = target;
      break;
    }
    curr = curr->next; // cycle
  }

  target->next = 0; // null ptr
  return target;
}

bool LinkedListUnregister(LLcontrol *ll, uint32_t structSize,
                          const void *LLtarget) {
  LinkedListNormal(ll, structSize);
  void **LLfirstPtr = (void **)(&ll->firstObject);

  LLheader *LLfirstCopy = *LLfirstPtr;

  LLheader *curr = (LLheader *)(*LLfirstPtr);
  while (curr) {
    if (curr->next && curr->next == LLtarget)
      break;
    curr = curr->next;
  }

  if (LLfirstCopy == LLtarget) {
    // target is the first one
    *LLfirstPtr = LLfirstCopy->next;
    return true;
  } else if (!curr)
    return false;

  LLheader *target = curr->next;
  curr->next = target->next; // remove reference

  return true;
}

bool LinkedListRemove(LLcontrol *ll, uint32_t structSize, void *LLtarget) {
  LinkedListNormal(ll, structSize);
  bool res = LinkedListUnregister(ll, structSize, LLtarget);
  free(LLtarget);
  return res;
}

void LinkedListPushFrontUnsafe(LLcontrol *ll, void *LLtarget) {
  void **LLfirstPtr = (void **)(&ll->firstObject);
  if (*LLfirstPtr == 0) {
    *LLfirstPtr = LLtarget;
    assert(((LLheader *)LLtarget)->next == 0);
    return;
  }

  void *next = *LLfirstPtr;
  *LLfirstPtr = LLtarget;
  LLheader *target = (LLheader *)(LLtarget);
  target->next = next;
}

void LinkedListDestroy(LLcontrol *ll, uint32_t structSize) {
  void **LLfirstPtr = (void **)(&ll->firstObject);
  LinkedListNormal(ll, structSize);

  LLheader *browse = (LLheader *)(*LLfirstPtr);
  while (browse) {
    void *next = browse->next;
    free(browse);
    browse = next;
  }

  *LLfirstPtr = 0;
}

void LinkedListInit(LLcontrol *ll, uint32_t structSize) {
  assert(ll->signature1 != LL_SIGNATURE_1);
  assert(ll->signature2 != LL_SIGNATURE_2);

  memset(ll, 0, sizeof(LLcontrol));

  ll->signature1 = LL_SIGNATURE_1;
  ll->signature2 = LL_SIGNATURE_2;
  ll->structSize = structSize;
}

void LinkedListTraverse(LLcontrol *ll, void(callback)(void *data, void *ctx),
                        void      *ctx) {
  LLheader *browse = (LLheader *)(ll->firstObject);
  while (browse) {
    callback(browse, ctx);
    browse = browse->next;
  }
}

void *LinkedListSearch(LLcontrol *ll, bool(isCorrect)(void *data, void *ctx),
                       void      *ctx) {
  LLheader *browse = (LLheader *)(ll->firstObject);
  while (browse) {
    if (isCorrect(browse, ctx))
      break;
    browse = browse->next;
  }
  return browse;
}

bool LinkedListSearchPtrCb(void *data, void *targetPtr) {
  return data == targetPtr;
}

void *LinkedListSearchPtr(LLcontrol *ll, void *targetPtr) {
  return LinkedListSearch(ll, LinkedListSearchPtrCb, targetPtr);
}
