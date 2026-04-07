#pragma once

#pragma once
#include <stddef.h>

void *malloc(size_t n);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t n);
void free (void *ptr);

void exit(int status) __attribute__((noreturn));
void abort(void)       __attribute__((noreturn));

double atof(const char *nptr);
int atoi(const char *s);
long atol(const char *s);
long strtol(const char *s, char **end, int base);

int system(char *command);

int  abs(int x);
long labs(long x);

// for screen clear command
//int system(const char *cmd);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1