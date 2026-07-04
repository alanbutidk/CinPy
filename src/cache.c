#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_CAP     64
#define DEFAULT_TIMEOUT 30

static CacheEntry *g_entries   = NULL;
static size_t      g_count     = 0;
static size_t      g_cap       = DEFAULT_CAP;
static int         g_timeout   = DEFAULT_TIMEOUT;

static CacheEntry  g_lru_head;
static CacheEntry  g_lru_tail;

static void lru_remove(CacheEntry *e)
{
    e->lru_prev->lru_next = e->lru_next;
    e->lru_next->lru_prev = e->lru_prev;
    e->lru_prev = NULL;
    e->lru_next = NULL;
}

static void lru_push_front(CacheEntry *e)
{
    e->lru_next = g_lru_head.lru_next;
    e->lru_prev = &g_lru_head;
    g_lru_head.lru_next->lru_prev = e;
    g_lru_head.lru_next = e;
}

int cinpy_cache_init(void)
{
    g_entries = calloc(g_cap, sizeof(CacheEntry));
    if (!g_entries)
        return -1;
    g_lru_head.lru_next = &g_lru_tail;
    g_lru_tail.lru_prev = &g_lru_head;
    g_lru_head.lru_prev = NULL;
    g_lru_tail.lru_next = NULL;
    return 0;
}

void cinpy_cache_config(size_t cap, int timeout_secs)
{
    if (cap > 0)          g_cap     = cap;
    if (timeout_secs > 0) g_timeout = timeout_secs;
}

static void evict_entry(CacheEntry *e)
{
    lru_remove(e);
    free(e->key);
    free(e->arg_types);
    memset(e, 0, sizeof(*e));
    g_count--;
}

static void evict_expired(void)
{
    time_t now = time(NULL);
    CacheEntry *e = g_lru_tail.lru_prev;
    while (e && e != &g_lru_head) {
        CacheEntry *prev = e->lru_prev;
        if (e->key && difftime(now, e->last_used) > g_timeout)
            evict_entry(e);
        e = prev;
    }
}

static void evict_lru_one(void)
{
    CacheEntry *lru = g_lru_tail.lru_prev;
    if (!lru || lru == &g_lru_head)
        return;
    evict_entry(lru);
}

static CacheEntry *find_slot(const char *key)
{
    for (size_t i = 0; i < g_cap; i++) {
        if (g_entries[i].key && strcmp(g_entries[i].key, key) == 0)
            return &g_entries[i];
    }
    return NULL;
}

static CacheEntry *find_empty(void)
{
    for (size_t i = 0; i < g_cap; i++) {
        if (!g_entries[i].key)
            return &g_entries[i];
    }
    return NULL;
}

CacheEntry *cinpy_cache_get(const char *key)
{
    evict_expired();
    CacheEntry *e = find_slot(key);
    if (!e)
        return NULL;
    e->last_used = time(NULL);
    lru_remove(e);
    lru_push_front(e);
    return e;
}

int cinpy_cache_put(const char *key, void *fn_ptr, ffi_cif *cif, ffi_type **arg_types)
{
    evict_expired();

    CacheEntry *e = find_slot(key);
    if (e) {
        free(e->arg_types);
        e->fn_ptr    = fn_ptr;
        e->cif       = *cif;
        e->arg_types = arg_types;
        e->last_used = time(NULL);
        lru_remove(e);
        lru_push_front(e);
        return 0;
    }

    if (g_count >= g_cap)
        evict_lru_one();

    e = find_empty();
    if (!e)
        return -1;

    e->key       = strdup(key);
    e->fn_ptr    = fn_ptr;
    e->cif       = *cif;
    e->arg_types = arg_types;
    e->last_used = time(NULL);
    lru_push_front(e);
    g_count++;
    return 0;
}

void cinpy_cache_evict_all(void)
{
    for (size_t i = 0; i < g_cap; i++) {
        if (g_entries[i].key) {
            lru_remove(&g_entries[i]);
            free(g_entries[i].key);
            free(g_entries[i].arg_types);
            memset(&g_entries[i], 0, sizeof(g_entries[i]));
        }
    }
    g_count = 0;
}

void cinpy_cache_free(void)
{
    cinpy_cache_evict_all();
    free(g_entries);
    g_entries = NULL;
}