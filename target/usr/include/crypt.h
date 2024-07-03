#ifndef _CRYPT_H
#define _CRYPT_H

#include <stddef.h>  // for size_t

#ifdef __cplusplus
extern "C" {
#endif

struct crypt_data {
    int initialized;
    char __buf[256];
};


char *crypt(const char *key, const char *salt);

char *crypt_r(const char *key, const char *salt, struct crypt_data *data);

#ifdef __cplusplus
}
#endif

#endif
