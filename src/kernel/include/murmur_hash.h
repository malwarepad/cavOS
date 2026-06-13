#include "types.h"

#ifndef MURMUR_HASH_H
#define MURMUR_HASH_H

uint64_t murmur_hash(const void *key, size_t len, uint64_t seed);

#endif
