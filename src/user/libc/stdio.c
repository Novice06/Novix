#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

static FILE _stdin  = { ._fd = STDIN_FILENO,  ._flags = _FILE_READ,  ._eof = 0, ._err = 0 };
static FILE _stdout = { ._fd = STDOUT_FILENO, ._flags = _FILE_WRITE, ._eof = 0, ._err = 0 };
static FILE _stderr = { ._fd = STDERR_FILENO, ._flags = _FILE_WRITE, ._eof = 0, ._err = 0 };

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

int remove(char *filename) {
    printf("TODO: remove() in libc\n");
    return 0; // __syscall1(13, (size_t) filename);    // implement this
}

int rename(const char *oldpath, const char *newpath) {
    printf("TODO: rename() in libc\n");
    return 0;
}

FILE *fopen(const char *path, const char *mode)
{
    if (!path || !mode) { errno = EINVAL; return NULL; }

    int flags = 0;
    int fflags = 0;

    // parse mode string
    char m = mode[0];
    int plus = (mode[1] == '+') || (mode[2] == '+');

    if (m == 'r') {
        flags  = plus ? O_RDWR : O_RDONLY;
        fflags = plus ? (_FILE_READ | _FILE_WRITE) : _FILE_READ;
    } else if (m == 'w') {
        flags  = plus ? (O_TRUNC | O_RDWR | O_CREAT) : (O_TRUNC | O_WRONLY | O_CREAT);
        fflags = plus ? (_FILE_READ | _FILE_WRITE) : _FILE_WRITE;
    } else if (m == 'a') {
        flags  = plus ? (O_APPEND | O_RDWR | O_CREAT) : (O_APPEND | O_WRONLY | O_CREAT);
        fflags = plus ? (_FILE_READ | _FILE_WRITE) : _FILE_WRITE;
    } else {
        errno = EINVAL;
        return NULL;
    }

    int fd = open(path, flags);
    if (fd < 0) return NULL; // errno already set by open()

    FILE *f = (FILE *)malloc(sizeof(FILE));
    if (!f) { close(fd); errno = ENOMEM; return NULL; }

    f->_fd    = fd;
    f->_flags = fflags;
    f->_eof   = 0;
    f->_err   = 0;

    return f;
}

int fclose(FILE *f)
{
    if (!f) { errno = EINVAL; return EOF; }

    // do not close the static standard streams
    int is_std = (f == stdin || f == stdout || f == stderr);

    int r = 0;
    if (!is_std) {
        r = close(f->_fd);
        free(f);
    }
    return r;
}

size_t fread(void *buf, size_t sz, size_t n, FILE *f)
{
    if (!f || !buf || !sz || !n) return 0;
    if (f->_eof || f->_err)      return 0;
    if (!(f->_flags & _FILE_READ)) { f->_err = 1; return 0; }

    size_t total = sz * n;
    ssize_t got  = read(f->_fd, buf, total);

    if (got < 0)              { f->_err = 1; return 0; }
    if (got == 0 || (size_t)got < total) f->_eof = 1;

    return (size_t)got / sz;  // full elements read
}

size_t fwrite(const void *buf, size_t sz, size_t n, FILE *f)
{
    if (!f || !buf || !sz || !n) return 0;
    if (!(f->_flags & _FILE_WRITE)) { f->_err = 1; return 0; }

    size_t total = sz * n;
    ssize_t written = write(f->_fd, buf, total);

    if (written < 0) { f->_err = 1; return 0; }
    return (size_t)written / sz;
}

char *fgets(char *buf, int n, FILE *f)
{
    if (!f || !buf || n <= 0) return NULL;
    if (f->_eof || f->_err)   return NULL;
    if (!(f->_flags & _FILE_READ)) { f->_err = 1; return NULL; }

    int i = 0;
    while (i < n - 1) {
        char c;
        ssize_t r = read(f->_fd, &c, 1);
        if (r < 0) { f->_err = 1; break; }
        if (r == 0) { f->_eof = 1; break; }
        buf[i++] = c;
        if (c == '\n') break;
    }

    if (i == 0) return NULL;
    buf[i] = '\0';
    return buf;
}


int fputc(int c, FILE *f)
{
    if (!f || !(f->_flags & _FILE_WRITE)) return EOF;
    unsigned char ch = (unsigned char)c;
    ssize_t r = write(f->_fd, &ch, 1);
    if (r != 1) { f->_err = 1; return EOF; }
    return (unsigned char)c;
}
int fputs(const char *s, FILE *f)
{
    if (!f || !s || !(f->_flags & _FILE_WRITE)) return EOF;
    size_t len = strlen(s);
    ssize_t r = write(f->_fd, s, len);
    if (r < 0) { f->_err = 1; return EOF; }
    return 0;
}
int feof(FILE *f) { return f ? f->_eof : 1; }
int ferror(FILE *f) { return f ? f->_err : 1; }


int vfprintf(FILE *f, const char *fmt, va_list ap)
{
    char buf[512];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
    ssize_t r = write(f->_fd, buf, (size_t)len);
    if (r < 0) { f->_err = 1; return -1; }
    return len;
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

int fseek(FILE *stream, long offset, int whence) {
    lseek(stream->_fd, offset, whence);
    return 0;
}

long ftell(FILE *stream) {
    return lseek(stream->_fd, 0, SEEK_CUR);
}

int fflush(FILE *stream) {
    // nothing to do gng it's not buffered
    return 0;
}

int sscanf(const char *str, const char *format, ...) {
    printf("TODO: sscanf is not implemented yet\n");
    exit(1);
}

static int _uitoa(unsigned long v, int base, char *out)
{
    static const char _hex[] = "0123456789abcdef";
    char tmp[32];
    int  len = 0;
    if (v == 0) { out[0] = '0'; return 1; }
    while (v) { tmp[len++] = _hex[v % (unsigned)base]; v /= (unsigned)base; }
    for (int i = 0; i < len; i++) out[i] = tmp[len - 1 - i];
    return len;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;
    char   tmp[32];
    int    tlen;

#define _EMIT(c) do { if (buf && pos + 1 < size) buf[pos] = (c); pos++; } while (0)

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { _EMIT(*p); continue; }
        p++;
        if (!*p) break;

        // width and zero-pad
        char pad    = ' ';
        int  width  = 0;
        int  is_long = 0;

        if (*p == '0') { pad = '0'; p++; }
        while (*p >= '1' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
        if (*p == 'l') { is_long = 1; p++; }

        switch (*p) {
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s) _EMIT(*s++);
                break;
            }
            case 'd': {
                long val = is_long ? va_arg(ap, long) : (long)va_arg(ap, int);
                if (val < 0) { _EMIT('-'); val = -val; }
                tlen = _uitoa((unsigned long)val, 10, tmp);
                for (int i = tlen; i < width; i++) _EMIT(pad);
                for (int i = 0; i < tlen; i++) _EMIT(tmp[i]);
                tlen = 0;
                break;
            }
            case 'u': {
                unsigned long val = is_long ? va_arg(ap, unsigned long)
                                            : (unsigned long)va_arg(ap, unsigned int);
                tlen = _uitoa(val, 10, tmp);
                for (int i = tlen; i < width; i++) _EMIT(pad);
                for (int i = 0; i < tlen; i++) _EMIT(tmp[i]);
                tlen = 0;
                break;
            }
            case 'x': {
                unsigned long val = is_long ? va_arg(ap, unsigned long)
                                            : (unsigned long)va_arg(ap, unsigned int);
                tlen = _uitoa(val, 16, tmp);
                for (int i = tlen; i < width; i++) _EMIT(pad);
                for (int i = 0; i < tlen; i++) _EMIT(tmp[i]);
                tlen = 0;
                break;
            }
            case 'p': {
                unsigned long val = (unsigned long)va_arg(ap, void *);
                _EMIT('0'); _EMIT('x');
                tlen = _uitoa(val, 16, tmp);
                for (int i = 0; i < tlen; i++) _EMIT(tmp[i]);
                tlen = 0;
                break;
            }
            case 'c': {
                _EMIT((char)va_arg(ap, int));
                break;
            }
            case '%': {
                _EMIT('%');
                break;
            }
            default: {
                _EMIT('%');
                if (is_long) _EMIT('l');
                _EMIT(*p);
                break;
            }
        }
    }

    if (buf && size > 0) buf[pos < size ? pos : size - 1] = '\0';
    return (int)pos;

#undef _EMIT
}


int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}

int vprintf(const char *fmt, va_list ap) {
    char buf[512];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    // clamp to actual buffer size to avoid overrun
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
    write(STDOUT_FILENO, buf, (size_t)len);
    return len;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int putchar(int c) {
    char ch = (char)c;
    write(STDOUT_FILENO, &ch, 1);
    return (unsigned char)ch;
}

int puts(const char *s) {
    write(STDOUT_FILENO, s, strlen(s));
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}

void perror(const char *s) {
    // "prefix: error message\n"
    if (s && s[0] != '\0') {
        write(STDERR_FILENO, s, strlen(s));
        write(STDERR_FILENO, ": ", 2);
    }
    const char *msg = strerror(errno);
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
}