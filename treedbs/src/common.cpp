extern "C" {

#include "config.h"
#include <pthread.h>

#include "murmurhash.h"

//#define CACHE_LINE 6
//#define CACHE_LINE_SIZE 64

#include <hre/user.h>

uint64_t MurmurHash64 (const void * key, int len, uint64_t seed) {
    return MurmurHash64<>(key, len, seed);
}

void* HREgetLocal(hre_key_t key) {
    static_assert(sizeof(pthread_key_t) <= sizeof(hre_key_t));
    return (void*)pthread_getspecific((pthread_key_t)key);
}

void HREsetLocal(hre_key_t key, void* package) {
    static_assert(sizeof(pthread_key_t) <= sizeof(hre_key_t));
    pthread_setspecific((pthread_key_t)key, package);
}

void HREcreateLocal(hre_key_t *key, void (*destructor)(void *)) {
    static_assert(sizeof(pthread_key_t) <= sizeof(hre_key_t));
    *key = 0;
    pthread_key_create((pthread_key_t*)key, destructor);
}

void* RTmallocZero(size_t size) {
    if(size==0) return NULL;
    void *tmp=calloc((size + CACHE_LINE_SIZE - 1) >> CACHE_LINE, CACHE_LINE_SIZE);
    return tmp;
}

void* RTalign(size_t align, size_t size) {
    void *ret = NULL;
    errno = posix_memalign(&ret, align, size);
    if (errno) {
    switch (errno) {
        case ENOMEM:
            fprintf(stderr, "out of memory on allocating %zu bytes aligned at %zu",
                  size, align);
        case EINVAL:
            fprintf(stderr, "invalid alignment %zu", align);
        default:
            fprintf(stderr, "unknown error allocating %zu bytes aligned at %zu",
                  size, align);
    }}
    HREassert (NULL != ret, "Alloc failed");
    Debug("allocated %zu aligned at %zu from system", size, align);
    return ret;
}
void RTfree(void* ptr) {
}

} // extern "C"
