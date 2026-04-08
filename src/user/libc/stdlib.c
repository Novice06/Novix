#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "_syscall.h"

// bump allocator via brk
static char *_heap = NULL;


void *malloc(size_t n) {
    if (!n) return NULL;
    if (!_heap) _heap = (char *)__sys_sbrk(0);
    n = (n + 15) & ~(size_t)15; // align 16
    char *p = _heap, *end = _heap + n;
    if ((char *)__sys_sbrk(n) < end) { errno = ENOMEM; return NULL; }
    _heap = end;
    return p;
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t n) {
    if (!ptr) return malloc(n);
    void *p = malloc(n);
    if (p) memcpy(p, ptr, n); // may copy garbage if n > old size, acceptable
    return p;
}

void free(void *ptr) { (void)ptr; } // no-op with bump allocator


// _exit in unistd.c
extern void _exit(int status) __attribute__((noreturn));

void exit(int status) { _exit(status); __builtin_unreachable(); }

void abort(void) { _exit(1); }

int system(char *command) {
    /*Novix doesnt have this !*/
    char* argv[] = {"shell", "-c", command, NULL};
    execve("/usr/bin/shell", argv , NULL);
    return 0;
}

long strtol(const char *s, char **end, int base) {
    while (*s == ' ') s++;
    int neg = (*s == '-'); if (neg || *s == '+') s++;
    if (base == 0) {
        if (*s == '0' && (s[1]=='x'||s[1]=='X')) { base=16; s+=2; }
        else if (*s == '0') { base=8; }
        else base=10;
    } else if (base==16 && *s=='0' && (s[1]=='x'||s[1]=='X')) s+=2;
    long v = 0;
    while (*s) {
        int d;
        if (*s>='0'&&*s<='9') d=*s-'0';
        else if (*s>='a'&&*s<='z') d=*s-'a'+10;
        else if (*s>='A'&&*s<='Z') d=*s-'A'+10;
        else break;
        if (d >= base) break;
        v = v*base + d; s++;
    }
    if (end) *end = (char *)s;
    return neg ? -v : v;
}

double atof(const char *s) {
    double value = 0.0;
    double frac_scale = 1.0;
    int sign = 1;
    int exp_sign = 1;
    int exp = 0;

    if (!s) {
        return 0.0;
    }

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (isdigit((unsigned char)*s)) {
        value = value * 10.0 + (double)(*s - '0');
        s++;
    }

    if (*s == '.') {
        s++;
        while (isdigit((unsigned char)*s)) {
            value = value * 10.0 + (double)(*s - '0');
            frac_scale *= 10.0;
            s++;
        }
    }

    value /= frac_scale;

    if (*s == 'e' || *s == 'E') {
        s++;
        if (*s == '-') {
            exp_sign = -1;
            s++;
        } else if (*s == '+') {
            s++;
        }

        while (isdigit((unsigned char)*s)) {
            exp = exp * 10 + (*s - '0');
            s++;
        }
    }

    while (exp > 0) {
        if (exp_sign > 0) {
            value *= 10.0;
        } else {
            value /= 10.0;
        }
        exp--;
    }

    return (double)sign * value;
}


int  atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }
int  abs (int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }