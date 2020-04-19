#include "config.h"
#include <pthread.h>

#include "murmurhash.h"
#include <mc-lib/atomics.h>
#include <hre/user.h>
#include <util-lib/util.h>

typedef size_t hre_key_t;

#undef HREassert
//#define HREassert(e, ...) do {  if(!(e)) { fprintf(stderr, "ASSERT FAILED[%s:%i]"); fprintf(stderr, "[" #e "]: " __VA_ARGS__); fprintf(stderr, "\n"); abort();} } while(false)
#define HREassert(e,...) \
    if (EXPECT_FALSE(!(e))) {\
        char buf[4096];\
        if (#__VA_ARGS__[0])\
            snprintf(buf, 4096, ": " __VA_ARGS__);\
        else\
            buf[0] = '\0';\
        fprintf(stderr, "[%s:%i] assertion \"%s\" failed%s\n", __FILE__, __LINE__, #e, buf);\
		abort();\
    }

//uint64_t MurmurHash64 (const void * key, int len, uint64_t seed);
void* HREgetLocal(hre_key_t key);
void HREsetLocal(hre_key_t key, void* package);
void HREcreateLocal(hre_key_t *key, void (*destructor)(void *));
void* RTmalloc(size_t size);
void* RTmallocZero(size_t size);
void* RTalign(size_t align, size_t size);
void RTfree(void* ptr);
void* RTalignZero(size_t align, size_t size);
void HREabort(int code);
void* HREalign(hre_region_t region,size_t align,size_t size);

int log_active(log_t log);
void log_message(log_t log,const char*file,int line,int errnum,const char *fmt,...);

/**
\brief These constructs prevent the compiler from optimizing (reordering) reads
    and writes to memory location. Strong order execution has to be guaranteed
    by the CPU for this to work. It seems like x86 is going to for the
    foreseeable future.
*/
#define atomic_read(v)      (*(volatile typeof(*v) *)(v))
#define atomic_write(v,a)   (*(volatile typeof(*v) *)(v) = (a))

int mix (int a, int b, int c);
uint64_t mix64 (uint64_t key);
