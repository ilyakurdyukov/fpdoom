#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

int fstat(int fd, struct stat *buf) {
	FILE *f = (FILE*)fd;
	long len, pos;
	pos = ftell(f);
	if (pos == -1) return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, pos, SEEK_SET);

	buf->st_size = len;
	buf->st_mtime = 0;
	return 0;
}

int stat(const char *fn, struct stat *buf) {
	FILE *f; int ret;
	f = fopen(fn, "rw");
	if (!f) return -1;
	ret = fstat((int)f, buf);
	fclose(f);
	return ret;
}

int open(const char *name, int flags, ...) {
	const char *smode;
	int i = 1, a;
	char fmode[4];
	switch (flags & (O_ACCMODE | O_CREAT | O_TRUNC)) {
	case O_RDONLY: a = 'r'; break;
	case O_RDWR: a = 'r'; break;
	case O_WRONLY | O_CREAT | O_TRUNC: a = 'w'; break;
	case O_RDWR | O_CREAT | O_TRUNC: a = 'w'; break;
	default: return -1;
	}

	fmode[0] = a;
	if ((flags & O_ACCMODE) == O_RDWR) fmode[i++] = '+';
	if (!O_BINARY || (flags & O_BINARY)) fmode[i++] = 'b';
	fmode[i] = 0;

	return (int)fopen(name, fmode);
}

int close(int fd) {
	return fclose((FILE*)fd);
}

int access(const char *name, int mode) {
	FILE *f; const char *fmode;
	switch (mode) {
	case F_OK:
	case R_OK:
		fmode = "rb"; break;
	default: return -1;
	}
	f = fopen(name, fmode);
	if (!f) return -1;
	fclose(f);
	return 0;
}

ssize_t read(int fd, void *buf, size_t count) {
	return fread(buf, 1, count, (FILE*)fd);
}

ssize_t write(int fd, const void *buf, size_t count) {
	return fwrite(buf, 1, count, (FILE*)fd);
}

off_t lseek(int fd, off_t off, int origin) {
	return fseek((FILE*)fd, off, origin);
}

// extra

int sys_errno = 0;
volatile int *__errno(void) { return &sys_errno; }

char *strerror(int errnum) { return "<unknown>"; }

void __assert2(const char *file, int line, const char *func, const char *msg) {
	fprintf(stderr, "!!! %s:%i %s: assert(%s) failed\n", file, line, func, msg);
	exit(255);
}

