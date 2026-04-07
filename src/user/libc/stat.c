#include <sys/stat.h>
#include "_syscall.h"

int mkdir(char *path, uint16_t mode) {
    return __sys_mkdir(path, mode);
}