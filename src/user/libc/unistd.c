#include <unistd.h>
#include "_syscall.h"

// no errno access

ssize_t read(int fd, void *buf, size_t n) {
    return (ssize_t)__sys_read(fd, buf, n);
}

ssize_t write(int fd, const void *buf, size_t n) {
    return (ssize_t)__sys_write(fd, buf, n);
}

pid_t getpid(void) {
    return (pid_t)__sys_getpid();
}

off_t lseek(int fd, off_t offset, int whence) {
    return __sys_seek(fd, offset, whence);
}

pid_t fork(void) {
    // return (pid_t)_sc1(_SCAL_FORK, 0);
}

void _exit(int status) {
    __sys_exit(status);
    _exit(status);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    (void)envp;
    return __sys_execve(path, argv);
}

int chdir(const char *path) {
    // return (int)_sc1(_SCAL_CHDIR, (long)path);
}

int unlink(const char *path) {
    return (int)__sys_unlink(path);
}

char *getcwd(char *buf, size_t size) {
    // long r = _sc2(_SCAL_GETCWD, (long)buf, (long)size);
    // return (r > 0) ? buf : NULL;
}