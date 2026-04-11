#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <_syscall.h>

#if UINT32_MAX == UINTPTR_MAX
    #define STACK_CHK_GUARD 0xe2dee396
#else
    #define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
__attribute__((noreturn))
void __stack_chk_fail(void) {
    for (;;); // TODO: Handle properly
}

void *sbrk(intptr_t increment) {
    return (void*) __sys_sbrk(increment);
}

off_t lseek(int fd, off_t offset, int whence) {
    return __sys_seek(fd, (int64_t)offset, whence);
}

pid_t fork(void) {
    return -1;
}

int execve(const char *path, char **argv, char **envp) {
    return __sys_execve(path, argv);
}

extern char **environ;

int execvp(const char *path, char **argv) {
    #if 0
    int contains_slash = strchr(path, '/') != NULL;
    if (contains_slash)
        return execve(path, argv, environ);
    char *pathlist = getenv("PATH");
    if (!pathlist) return -1;
    while (*pathlist) {
        char* start = pathlist;
        while (*pathlist && *pathlist != ':') pathlist++;
        char* dir = strndup(start, pathlist-start);
        char* pattern = "%s/%s";
        size_t len = sprintf(NULL, pattern, dir, path);
        char* full_path = (char*) malloc(len + 1);
        sprintf(full_path, pattern, dir, path);
        int f = open(full_path, 0, 0);
        if (f < 0) {
            free(dir);
            return -1;
        }
        execve(full_path, argv, environ);
        free(dir);
        if (!*pathlist) break;
        pathlist++;
    }
    #endif
    return -1;
}

int chdir(const char *path) {
    return -1;
}

char *getcwd(char *buf, size_t size) {
    return -1;
}

int ftruncate(int fd, off_t length) {
    return -1;
}

int dup2(int oldfd, int newfd) {
    return -1;
}
