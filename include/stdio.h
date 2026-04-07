#pragma once
#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

typedef struct {
    int _fd;
    int _flags;
    int _eof;
    int _err;
} FILE;

#define SEEK_SET 1 << 0
#define SEEK_CUR 1 << 1
#define SEEK_END 1 << 2

#define _FILE_READ  0x01
#define _FILE_WRITE 0x02

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int remove(char *filename);
int rename(const char *oldpath, const char *newpath);
int sscanf(const char *str, const char *format, ...);

void perror(const char *s);

FILE *fopen(const char *path, const char *mode);

int fclose(FILE *f);
size_t fread(void *buf, size_t sz, size_t n, FILE *f);
size_t fwrite(const void *buf, size_t sz, size_t n, FILE *f);
char *fgets(char *buf, int n, FILE *f);
int fputc(int c, FILE *f);
int fputs(const char *s, FILE *f);
int feof(FILE *f);
int ferror(FILE *f);
int fprintf(FILE *f, const char *fmt, ...);
int vfprintf(FILE *f, const char *fmt, va_list ap);
long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fflush(FILE *stream);

int puts(const char *s);
int putchar(int c);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);