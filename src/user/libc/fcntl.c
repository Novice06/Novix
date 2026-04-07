#include <fcntl.h>
#include <errno.h>
#include "_syscall.h"

int open(const char *path, int flags, ...)
{
    long r = __sys_open(path, flags);
    if (r < 0) { errno = (int)-r; return -1; }
    return (int)r;
}

int close(int fd)
{
    long r = __sys_close(fd);
    if (r < 0) { errno = (int)-r; return -1; }
    return 0;
}