#ifndef __SYS_STAT
#define __SYS_STAT

#ifdef __cplusplus
extern "C" {
#endif

int mkdir(const char*, int);

struct stat {
	int st_mode, st_size, st_mtime;
};

int fstat(int, struct stat*);
int stat(const char*, struct stat*);
int fstatat(int, const char*, struct stat*, int);

#define S_IFDIR 0040000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200

#ifdef __cplusplus
}
#endif
#endif
