#include <atomic>

static std::atomic<uint64_t> alloc_cnt_ = 0;
static std::atomic<uint64_t> free_cnt_ = 0;

uint64_t safe_alloc_cnt()
{
    return alloc_cnt_;
}

uint64_t safe_free_cnt()
{
    return free_cnt_;
}

void* safe_malloc(size_t size)
{
    ++alloc_cnt_;
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "malloc failed!\n");
        exit(-1);
    }
    return ptr;
}

void* safe_realloc(void* oldptr, size_t newsize, size_t oldsize)
{
    ++alloc_cnt_;
    ++free_cnt_;
    void* ptr = realloc(oldptr, newsize);
    if (!ptr) {
        fprintf(stderr, "realloc failed!\n");
        exit(-1);
    }
    if (newsize > oldsize) {
        memset((char*)ptr + oldsize, 0, newsize - oldsize);
    }
    return ptr;
}

void* safe_zalloc(size_t size)
{
    ++alloc_cnt_;
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "malloc failed!\n");
        exit(-1);
    }
    memset(ptr, 0, size);
    return ptr;
}

void safe_free(void* ptr)
{
    if (ptr) {
        free(ptr);
        ptr = NULL;
        ++free_cnt_;
    }
}
