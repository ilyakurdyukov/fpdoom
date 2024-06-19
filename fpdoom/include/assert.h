#ifndef __ASSERT
#define __ASSERT

#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#define assert(e) ((e) ? (void)0 : __assert2(__FILE__, __LINE__, __func__, #e))
#endif

__attribute__((noreturn))
void __assert2(const char *, int, const char *, const char *);

#endif
