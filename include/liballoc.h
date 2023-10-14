#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#include "pmm.h"
#include "types.h"

#define malloc(size) buddy_malloc(buddy, size)
#define free(ptr) buddy_free(buddy, ptr)
#define realloc(ptr, requested_size) buddy_realloc(buddy, ptr, requested_size)
#define calloc(members_count, member_size)                                     \
  buddy_calloc(buddy, members_count, member_size)

#endif
