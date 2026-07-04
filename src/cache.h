#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <time.h>
#include <ffi.h>

typedef struct CacheEntry {
    char            *key;
    void            *fn_ptr;
    ffi_cif          cif;
    ffi_type       **arg_types;
    time_t           last_used;
    struct CacheEntry *lru_prev;
    struct CacheEntry *lru_next;
} CacheEntry;

int         cinpy_cache_init(void);
void        cinpy_cache_config(size_t cap, int timeout_secs);
CacheEntry *cinpy_cache_get(const char *key);
int         cinpy_cache_put(const char *key, void *fn_ptr, ffi_cif *cif, ffi_type **arg_types);
void        cinpy_cache_evict_all(void);
void        cinpy_cache_free(void);

#endif