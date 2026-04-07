#include <string.h>
#include <stdlib.h>
#include <errno.h>

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}
void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    if (d < s) for (size_t i = 0; i < n; i++) d[i] = s[i];
    else for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    return dst;
}
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}
int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < n; i++)
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    return 0;
}
// who designed this, shouldnt this be in stdlib.c......

char *strerror(int errnum) {
    switch (errnum) {
        case 0:      return "success";
        case EPERM:  return "operation not permitted";
        case ENOENT: return "no such file or directory";
        case ESRCH:  return "no such process";
        case EINTR:  return "interrupted system call";
        case EIO:    return "i/o error";
        case EBADF:  return "bad file descriptor";
        case ENOMEM: return "out of memory";
        case EACCES: return "permission denied";
        case EFAULT: return "bad address";
        case EEXIST: return "file exists";
        case ENODEV: return "no such device";
        case ENOTDIR:return "not a directory";
        case EISDIR: return "is a directory";
        case EINVAL: return "invalid argument";
        case EMFILE: return "too many open files";
        case ENOSPC: return "no space left on device";
        case EPIPE:  return "broken pipe";
        case ERANGE: return "result too large";
        case ENOSYS: return "function not implemented";
        default:     return "unknown error";
    }
}

size_t strlen(const char *s) {
    size_t n = 0; while (s[n]) n++; return n;
}
char *strcpy(char *dst, const char *src) {
    char *r = dst; while ((*dst++ = *src++)); return r;
}
char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}
char *strcat(char *dst, const char *src) {
    char *r = dst; while (*dst) dst++; while ((*dst++ = *src++)); return r;
}
char *strncat(char *dst, const char *src, size_t n) {
    char *r = dst; while (*dst) dst++;
    for (size_t i = 0; i < n && src[i]; i++) *dst++ = src[i];
    *dst = '\0'; return r;
}
int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}
char *strchr(const char *s, int c) {
    for (; *s; s++) if (*s == (char)c) return (char *)s;
    return NULL;
}
char *strrchr(const char *s, int c) {
    char *r = NULL;
    for (; *s; s++) if (*s == (char)c) r = (char *)s;
    return r;
}
char *strstr(const char *hay, const char *needle) {
    if (!*needle) return (char *)hay;
    size_t nl = strlen(needle);
    for (; *hay; hay++)
        if (*hay == *needle && strncmp(hay, needle, nl) == 0)
            return (char *)hay;
    return NULL;
}

static char *_tok;
char *strtok(char *s, const char *delim) {
    if (s) _tok = s;
    if (!_tok) return NULL;
    while (*_tok && strchr(delim, *_tok)) _tok++;
    if (!*_tok) { _tok = NULL; return NULL; }
    char *tok = _tok;
    while (*_tok && !strchr(delim, *_tok)) _tok++;
    if (*_tok) *_tok++ = '\0'; else _tok = NULL;
    return tok;
}

char *strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}