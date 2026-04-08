#pragma once
#include <sys/types.h>
#define STDIN_FILENO  -9 // invalid
#define STDOUT_FILENO -1
#define STDERR_FILENO -2


// i/o
ssize_t read(int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);

off_t lseek(int fd, off_t offset, int whence);

// filesystem
int chdir (const char *path);
int mkdir (const char *path);
char *getcwd (char *buf, size_t size);
int unlink(const char *path);

// process
pid_t getpid (void);
pid_t fork (void);
void _exit (int status) __attribute__((noreturn));
int execve (const char *path, char *const argv[], char *const envp[]);