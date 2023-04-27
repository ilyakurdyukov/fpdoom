#ifndef __STDLIB
#define __STDLIB

#include <stddef.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int abs(int num) {
#ifdef __thumb__
	int sign = num >> 31;
	return (num ^ sign) - sign;
#else
	return num < 0 ? -num : num;
#endif
}

static inline long labs(long num) {
#ifdef __thumb__
	long sign = num >> (sizeof(long) * 8 - 1);
	return (num ^ sign) - sign;
#else
	return num < 0 ? -num : num;
#endif
}

int atoi(const char*);
long atol(const char*);
long strtol(const char*, char**, int);
unsigned long strtoul(const char*, char**, int);

__attribute__((noreturn))
void exit(int);
int atexit(void(*)(void));

#define RAND_MAX 0x7fffffff
void srand(unsigned);
int rand(void);

void qsort(void*, size_t, size_t,
		int (*)(const void*, const void*));
void* bsearch(const void*, const void*, size_t, size_t,
		int (*)(const void*, const void*));

#ifdef __cplusplus
}
#endif

#endif
