#include <stddef.h>

#ifdef __APPLE__
extern void sys_icache_invalidate(const void *addr, size_t len);
#else
extern void __clear_cache(void *begin, void *end);
#endif

void flush_icache(void *start, void *end) {
#ifdef __APPLE__
    sys_icache_invalidate(start, (size_t)((const char *)end - (const char *)start));
#else
    __clear_cache(start, end);
#endif
}
