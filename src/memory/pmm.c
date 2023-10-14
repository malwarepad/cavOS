#define BUDDY_ALLOC_IMPLEMENTATION
#include "../../include/pmm.h"

// Memory allocation system based off https://github.com/spaskalev/buddy_alloc
// Copyright (C) 2023 Panagiotis, Stanislav Paskalev

// void *data = buddy_malloc(buddy, 2048);
// buddy_free(buddy, data);

void initiateBitmap() {
  void  *start = INVALID_BLOCK;
  size_t arena_size;

  for (int i = 0; i < memoryMapCnt; i++) {
    multiboot_memory_map_t *entry = memoryMap[i];
    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
      if (start == INVALID_BLOCK || arena_size < entry->len) {
        start = entry->addr;
        arena_size = entry->len;
        debugf("%lx\n", entry->len);
      }
    }
  }
  debugf("Memory allocator initialization: stat=%lx len=%lx\n", start,
         arena_size);

  if (start == INVALID_BLOCK) {
    printf("[+] Memory allocator: Cannot find area to put arena at!");
  }

  buddy = buddy_embed(start, arena_size);
}
