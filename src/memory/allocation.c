#include "../../include/allocation.h"

// Real-Time Clock implementation
// Copyright (C) 2023 Panagiotis

static size_t memory_pool_size = 0;
static char  *memory_pool = NULL;
static char  *memory_ptr = NULL;

struct MemoryBlock {
  size_t              size;
  struct MemoryBlock *next;
};

static void splitBlock(struct MemoryBlock *block, size_t size) {
  struct MemoryBlock *newBlock = (struct MemoryBlock *)((char *)block + size);
  newBlock->size = block->size - size - sizeof(struct MemoryBlock);
  newBlock->next = block->next;
  block->size = size;
  block->next = newBlock;
}

void init_memory() {
  // if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
  //   uint64                  available_memory = 0;
  //   multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
  //   multiboot_memory_map_t *mmap_end =
  //       mmap + (mbi->mmap_length / sizeof(multiboot_memory_map_t));

  //   while (mmap < mmap_end) {
  //     if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
  //       available_memory += mmap->len_high;
  //     }
  //     mmap = (multiboot_memory_map_t *)((uintptr_t)mmap + mmap->size +
  //                                       sizeof(mmap->size));
  //   }

  //   memory_pool_size = available_memory;
  //   memory_pool = (char *)mbi->mem_lower;
  //   memory_ptr = memory_pool;
  // }
}

void *malloc(size_t size) {
  size = (size + sizeof(struct MemoryBlock) - 1) / sizeof(struct MemoryBlock) *
         sizeof(struct MemoryBlock);

  struct MemoryBlock *current = (struct MemoryBlock *)memory_pool;
  struct MemoryBlock *prev = NULL;

  while (current) {
    if (current->size >= size) {
      if (current->size > size + sizeof(struct MemoryBlock)) {
        splitBlock(current, size);
      }
      if (prev) {
        prev->next = current->next;
      } else {
        memory_ptr = (char *)current + current->size;
      }
      return (char *)current + sizeof(struct MemoryBlock);
    }
    prev = current;
    current = current->next;
  }

  return NULL; // out of memory
}

// Deallocate memory
void free(void *ptr) {
  if (ptr == NULL)
    return; // no NULL pointers

  struct MemoryBlock *block =
      (struct MemoryBlock *)((char *)ptr - sizeof(struct MemoryBlock));
  block->next = (struct MemoryBlock *)memory_ptr;
  memory_ptr = (char *)block;
}