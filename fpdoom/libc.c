#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

sighandler_t signal(int signum, sighandler_t handler) {
	(void)signum; (void)handler;
	return NULL;
}

#define PREFIX(name) name
#include "libc/printf.h"
#include "libc/malloc.h"
#undef PREFIX

int getchar(void) {
	return fgetc(stdin);
}

int putchar(int ch) {
	return fputc(ch, stdout);
}

int puts(const char *str) {
	size_t len = strlen(str);
	if (fwrite(str, 1, len, stdout) != len) return EOF;
	return putchar('\n');
}

int fputs(const char *str, FILE *f) {
	size_t len = strlen(str);
	return fwrite(str, 1, len, f) != len ? EOF : 0;
}

void exit(int code) {
	fprintf(stderr, "!!! exit(%d)\n", code);
	for (;;);
}

int atexit(void (*func)(void)) {
	(void)func;
	return -1;
}

FILE *_stdin = NULL;
FILE *_stdout = NULL;
FILE *_stderr = NULL;

struct _IO_FILE {
	uint16_t handle, flags, pos, len;
	char *buf;
};

#include "cmd_def.h"
#include "usbio.h"

void _debug_msg(const char *msg) {
	union { uint8_t u8[4]; uint16_t u16[2]; } buf;
	int len = strlen(msg);
	unsigned tmp;
	if (len > 255) len = 255;
	tmp	= CMD_MESSAGE | len << 8;
	buf.u16[0] = tmp;
	tmp += CHECKSUM_INIT;
	tmp = fastchk16(tmp, msg, len);
	buf.u16[1] = tmp;
	usb_write(buf.u16, 4);
	usb_write(msg, len);
}

void _sig_intdiv(void) {
	_debug_msg("divide by zero");
	exit(252);
}

void _sig_intovf(void) {
	_debug_msg("divide overflow");
	exit(252);
}

static FILE* _file_alloc(int handle, int flags) {
	// int len = (flags ^ _IO_PIPE) & (_IO_WRITE | _IO_PIPE) ? BUFSIZ;
	int len = BUFSIZ;
	FILE *f = malloc(sizeof(FILE) + len);
	if (f) {
		f->handle = handle;
		f->flags = flags;
		f->pos = 0;
		f->len = flags & _IO_WRITE ? len : 0;
		f->buf = len ? (char*)(f + 1) : NULL;
	}
	return f;
}

void _stdio_init(void) {
	stdin = _file_alloc(0, _IO_LINEBUF | _IO_PIPE);
	stdout = _file_alloc(1, _IO_LINEBUF | _IO_WRITE | _IO_PIPE);
	stderr = _file_alloc(2, _IO_LINEBUF | _IO_WRITE | _IO_PIPE);
}

int _argv_init(char ***argvp, int skip) {
	union { uint8_t u8[6]; uint16_t u16[3]; } buf;
	char *mem = NULL; char **argv;
	unsigned tmp; int argc, i, size;

	buf.u16[0] = CMD_GETARGS | (_ARGV_GET_ARGC | _ARGV_GET_SIZE) << 8;
	buf.u16[1] = skip;
	buf.u16[2] = fastchk16(CHECKSUM_INIT, buf.u16, 4);
	usb_write(buf.u16, 6);
	usb_read(buf.u16, 6, USB_WAIT);

	tmp = fastchk16(CHECKSUM_INIT, buf.u16, 4);
	if (buf.u16[2] != tmp) {
		_debug_msg("argv: bad checksum");
		exit(253);
	}

	argc = buf.u16[0];
	size = buf.u16[1];
	if (!argc) {
		*argvp = NULL;
		return 0;
	}
	argv = (char**)malloc(argc * sizeof(char*) + size);
	if (!argv) {
		_debug_msg("argv: malloc failed");
		exit(252);
	}
	*argvp = argv;
	mem = (char*)(argv + argc);
	
	buf.u16[0] = CMD_GETARGS | _ARGV_GET_ARGV << 8;
	buf.u16[1] = skip;
	buf.u16[2] = fastchk16(CHECKSUM_INIT, buf.u16, 4);
	usb_write(buf.u16, 6);
	usb_read(buf.u16, 2, USB_WAIT);
	usb_read(mem, size, USB_WAIT);

	tmp = fastchk16(CHECKSUM_INIT, mem, size);
	if (buf.u16[0] != tmp) {
		_debug_msg("argv: bad checksum");
		exit(253);
	}
	if (mem[size - 1]) {
		_debug_msg("argv: recieved wrong data");
		exit(252);
	}

	for (i = 0; i < argc; i++) {
		int len;
		if (size <= 0) {
			_debug_msg("argv: recieved wrong data");
			exit(252);
		}
		argv[i] = mem;
		mem += len = strlen(mem) + 1;
		size -= len;
	}
	if (size) {
		_debug_msg("argv: recieved wrong data");
		exit(252);
	}
	return argc;
}

static size_t _read_raw(int handle, size_t size, void *dst) {
	union { uint8_t u8[6]; uint16_t u16[3]; } buf;
	unsigned tmp; int ret;

	if (!size) return 0;
	tmp = CMD_FREAD | handle << 8;
	buf.u16[0] = tmp;
	buf.u16[1] = size;
	tmp += CHECKSUM_INIT + size;
	tmp = tmp + (tmp >> 16);
	buf.u16[2] = tmp;
	usb_write(buf.u16, 6);
	usb_read(buf.u16, 4, USB_WAIT);
	ret = buf.u16[0];
	tmp = CHECKSUM_INIT + ret;
	tmp = (tmp + (tmp >> 16)) & 0xffff;
	if ((unsigned)ret > size) {
		_debug_msg("fread: ret > size");
		exit(253);
	}
	if (ret) usb_read(dst, ret, USB_WAIT);
	tmp = fastchk16(tmp, dst, ret);
	if (buf.u16[1] != tmp) {
		_debug_msg("fread: bad checksum");
		exit(253);
	}
	return ret;
}

static size_t _write_raw(int handle, size_t size, const void *src) {
	union { uint8_t u8[6]; uint16_t u16[3]; } buf;
	unsigned tmp;

	tmp = CMD_FWRITE | handle << 8;
	buf.u16[0] = tmp;
	buf.u16[1] = size;
	tmp += CHECKSUM_INIT + size;
	tmp = fastchk16(tmp, src, size);
	buf.u16[2] = tmp;
	usb_write(buf.u16, 6);
	usb_write(src, size);
	usb_read(buf.u16, 2, USB_WAIT);
	return buf.u16[0];
}

static int _close_raw(int handle) {
	union { uint8_t u8[4]; uint16_t u16[2]; } buf;
	unsigned tmp;

	tmp = CMD_FCLOSE | handle << 8;
	buf.u16[0] = tmp;
	tmp += CHECKSUM_INIT;
	tmp = tmp + (tmp >> 16);
	buf.u16[1] = tmp;
	usb_write(buf.u16, 4);
	usb_read(buf.u8, 1, USB_WAIT);
	return (char)buf.u8[0];
}

int fflush(FILE *f) {
	unsigned pos;
	if (!f || !(f->flags & _IO_WRITE)) return EOF;
	pos = f->pos;
	f->pos = 0;
	if (pos) _write_raw(f->handle, pos, f->buf);
	return 0;
}

int fgetc(FILE *f) {
	unsigned pos, len;

	if (!f || (f->flags & _IO_WRITE)) return EOF;

	pos = f->pos;
	len = f->len;
	if (pos < len) {
		f->pos = pos + 1;
		return f->buf[pos];
	}
	if (f->buf) {
		len = _read_raw(f->handle, BUFSIZ, f->buf);
		if (!len) return EOF;
		f->pos = 1;
		f->len = len;
		return f->buf[0];
	} else {
		uint8_t buf[1];
		len = _read_raw(f->handle, 1, buf);
		return len ? buf[0] : EOF;
	}
}

int fputc(int ch, FILE *f) {
	int pos;

	if (!f || !(f->flags & _IO_WRITE)) return EOF;
	pos = f->pos;
	f->buf[pos++] = ch;
	f->pos = pos;
	if (pos == f->len ||
			((f->flags & _IO_LINEBUF) && ch == '\n')) {
		int ret;
		f->pos = 0;
		ret = _write_raw(f->handle, pos, f->buf);
		if (!ret) return EOF;
	}
	return ch;
}

size_t fread(void *dst, size_t size, size_t count, FILE *f) {
	uint8_t *d = (uint8_t*)dst;
	size_t ret = 0;
	uint64_t size2;
	size2 = (uint64_t)size * count;
	if (size2 >> 32) return EOF;
	count = size2;

	if (!f || (f->flags & _IO_WRITE)) return EOF;
	if (f->buf)
	while (count) {
		unsigned n, pos = f->pos, len = f->len;
		char *buf;
		n = len - pos;
		if (!n) {
			len = BUFSIZ;
			if (count >= len) {
				len = MAX_IOSIZE;
				if (len > count) len = count;
				ret += n = _read_raw(f->handle, len, d); d += n;
				count -= n;
				if (n == len) continue;
				break;
			}
			n = _read_raw(f->handle, len, f->buf);
			f->len = n;
			f->pos = pos = 0;
			if (!n) break;
			if (n < len && len < count) count = len;
		}
		buf = f->buf + pos;
		if (n > count) n = count;
		f->pos = pos += n;
		count -= n;
		memcpy(d, buf, n); d += n;
		ret += n;
	}
	else
	while (count) {
		unsigned n, len = MAX_IOSIZE;
		if (len > count) len = count;
		ret += n = _read_raw(f->handle, len, d); d += n;
		count -= n;
		if (len != n) break;
	}
	if (size > 1) ret /= size;
	return ret;
}

size_t fwrite(const void *src, size_t size, size_t count, FILE *f) {
	const uint8_t *s = (const uint8_t*)src;
	size_t ret = 0;
	uint64_t size2;
	size2 = (uint64_t)size * count;
	if (size2 >> 32) return EOF;
	count = size2;

	if (!f || !(f->flags & _IO_WRITE)) return EOF;
	while (count) {
		unsigned pos = f->pos, len = f->len;
		unsigned n = len - pos;
		char *buf = f->buf + pos;
		if (!pos && count >= len) {
			len = MAX_IOSIZE;
			if (len > count) len = count;
			ret += n = _write_raw(f->handle, len, s); s += n;
			count -= n;
			if (n == len) continue;
			break;
		}
		if (n > count) n = count;
		f->pos = pos += n;
		count -= n;
		memcpy(buf, s, n); s += n;
		if (pos == len) {
			len = _write_raw(f->handle, len, f->buf);
			f->pos = 0;
			if (len != pos) break;
		}
		ret += n;
	}
	if (size > 1) ret /= size;
	return ret;
}

int fseek(FILE *f, long offset, int origin) {
	union { uint8_t u8[10]; uint16_t u16[5]; uint32_t u32[2]; } buf;
	unsigned tmp;

	if (!f || (unsigned)origin > 2) return EOF;
	if (f->flags & _IO_WRITE) fflush(f);
	else {
		if (origin == SEEK_CUR)
			offset -= f->len - f->pos;
		f->pos = f->len = 0;
	}

	buf.u16[0] = CMD_FSEEK | f->handle << 8;
	buf.u16[1] = origin;
	buf.u32[1] = offset;
	tmp = fastchk16(CHECKSUM_INIT, buf.u8, 8);
	buf.u16[4] = tmp;
	usb_write(buf.u16, 10);
	usb_read(buf.u8, 1, USB_WAIT);
	return (char)buf.u8[0];
}

long ftell(FILE *f) {
	union { uint8_t u8[4]; uint16_t u16[2]; uint32_t u32[1]; } buf;
	unsigned tmp; long ret;

	if (!f) return -1;
	if (f->flags & _IO_WRITE) fflush(f);

	tmp = CMD_FTELL | f->handle << 8;
	buf.u16[0] = tmp;
	tmp += CHECKSUM_INIT;
	tmp = tmp + (tmp >> 16);
	buf.u16[1] = tmp;
	usb_write(buf.u16, 4);
	usb_read(buf.u32, 4, USB_WAIT);
	ret = (int32_t)buf.u32[0];
/*
	if (!(f->flags & _IO_WRITE) && ret != EOF)
		ret -= f->len - f->pos;
*/
	return ret;
}

FILE *fopen(const char *name, const char *mode) {
	union { uint8_t u8[6]; uint16_t u16[3]; } buf;
	int flags = 0; size_t len;
	unsigned tmp; FILE *f;

	tmp = *mode;
	if (tmp != 'r') {
		if (tmp == 'w') flags = _IO_WRITE;
		else if (tmp == 'a') flags = _IO_WRITE | _IO_APPEND;
		else return NULL;
	}

	len = strlen(name);
	if (len >= MAX_IOSIZE) return NULL;
	tmp = CMD_FOPEN | flags << 8;
	buf.u16[0] = tmp;
	buf.u16[1] = len;
	tmp += CHECKSUM_INIT + len;
	tmp = fastchk16(tmp, name, len);
	buf.u16[2] = tmp;
	usb_write(buf.u16, 6);
	usb_write(name, len);
	usb_read(buf.u8, 1, USB_WAIT);
	tmp = buf.u8[0];

	if (tmp == 0xff) return NULL;
	f = _file_alloc(tmp, flags);
	if (!f) _close_raw(tmp);
	return f;
}

int fclose(FILE *f) {
	int handle;
	if (!f) return EOF;
	if (f->flags & _IO_WRITE) fflush(f);
	handle = f->handle;
	free(f);
	return _close_raw(handle);
}

void setbuf(FILE *f, char *buf) {
	(void)f; (void)buf;
	// nothing
}

