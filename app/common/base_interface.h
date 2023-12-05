#ifndef BASE_INTERFACE_H_
#define BASE_INTERFACE_H_

#include <algorithm>
#include <memory>
#include <stdio.h>

#ifdef PRINT_DEBUG
#define printd(...) printf(__VA_ARGS__)
#else
#define printd(...)
#endif

uint64_t safe_alloc_cnt();
uint64_t safe_free_cnt();

void* safe_malloc(size_t size);
void* safe_realloc(void* oldptr, size_t newsize, size_t oldsize);
void* safe_zalloc(size_t size);
void safe_free(void* ptr);

#define SAFE_ALLOC(ptr, size)                                                                    \
    do {                                                                                         \
        *(void**)&(ptr) = safe_zalloc(size);                                                     \
        printd("alloc(%p, size=%llu)\tat [%s:%d:%s]\n", ptr, (unsigned long long)size, __FILE__, \
               __LINE__, __FUNCTION__);                                                          \
    } while (0)

#define SAFE_ALLOC_SIZEOF(ptr) SAFE_ALLOC(ptr, sizeof(*(ptr)))

#define SAFE_FREE(ptr)                                                                    \
    do {                                                                                  \
        if (ptr) {                                                                        \
            safe_free(ptr);                                                               \
            printd("free( %p )\tat [%s:%d:%s]\n", ptr, __FILE__, __LINE__, __FUNCTION__); \
            ptr = NULL;                                                                   \
        }                                                                                 \
    } while (0)

#endif
