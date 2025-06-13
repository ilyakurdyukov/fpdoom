#ifndef __STDIO
#define __STDIO

#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _IO_FILE;
typedef struct _IO_FILE FILE;

#define BUFSIZ 1024

#define stdin  _stdin
#define stdout _stdout
#define stderr _stderr

extern FILE *stdin, *stdout, *stderr;

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int getchar(void);
int putchar(int);
int fgetc(FILE*);
int ungetc(int, FILE*);
int feof(FILE*);
int fputc(int, FILE*);
int puts(const char*);
int fputs(const char*, FILE*);
char *fgets(char*, int, FILE*);

FILE *fopen(const char*, const char*);
int fseek(FILE*, long, int);
long ftell(FILE*);
size_t fread(void*, size_t, size_t, FILE*);
size_t fwrite(const void*, size_t, size_t, FILE*);
int fclose(FILE*);
int fflush(FILE*);
void setbuf(FILE*, char*);

#ifndef __GNUC__
#define __attribute__(attr)
#endif

__attribute__((__format__(__printf__, 1, 2)))
int printf(const char*, ...);
__attribute__((__format__(__printf__, 2, 3)))
int sprintf(char*, const char*, ...);
__attribute__((__format__(__printf__, 3, 4)))
int snprintf(char*, size_t, const char*, ...);
__attribute__((__format__(__printf__, 2, 3)))
int fprintf(FILE*, const char*, ...);
int vprintf(const char*, va_list);
int vsprintf(char*, const char*, va_list);
int vsnprintf(char*, size_t, const char*, va_list);
int vfprintf(FILE*, const char*, va_list);

__attribute__((__format__(__scanf__, 2, 3)))
int sscanf(const char *, const char *, ...);
__attribute__((__format__(__scanf__, 2, 3)))
int fscanf(FILE*, const char*, ...);

#ifdef __cplusplus
}
#endif

#endif
