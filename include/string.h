#pragma once
#include <stddef.h>

char *strerror(int errnum);

void *memcpy (void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset (void *s,   int c,           size_t n);
int memcmp (const void *a, const void *b, size_t n);

size_t strlen (const char *s);
char *strcpy (char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat (char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t n);
int  strcmp (const char *a, const char *b);
int  strncmp(const char *a, const char *b, size_t n);
char *strchr (const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr (const char *hay, const char *needle);
char *strtok (char *s, const char *delim);
char *strdup (const char *s);