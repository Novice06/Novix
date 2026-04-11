#include <sys/stat.h>
#include <_syscall.h>

int mkdir(char *path, mode_t mode) {
    return __sys_mkdir(path, mode);
}
