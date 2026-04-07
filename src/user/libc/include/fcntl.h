#pragma once
#include <sys/types.h>

// open flags (needs to vfs.h)
#define O_RDONLY 1 << 0
#define O_WRONLY 1 << 1
#define O_RDWR  1 << 2
#define O_NONBLOCK 1 << 3
#define O_CREAT 1 << 4

int open(const char *path, int flags, ...);
int close(int fd);