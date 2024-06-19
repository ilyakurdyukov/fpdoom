#ifndef __UNISTD
#define __UNISTD

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int open(const char*, int, ...);
int close(int);
ssize_t read(int, void*, size_t);
ssize_t write(int, const void*, size_t);
off_t lseek(int, off_t, int);
char *getcwd(char*, size_t);
int chdir(const char*);

int access(const char*, int);

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#ifdef __cplusplus
}
#endif
#endif
