#ifndef __ERRNO
#define __ERRNO

#ifdef __cplusplus
extern "C" {
#endif

extern int *__errno(void);
#define errno (*__errno())

#define ENOENT 2
#define EINVAL 22

#ifdef __cplusplus
}
#endif
#endif
