#pragma once

#include <stdint.h>

int __sys_open(const char* path, uint16_t mode);
int64_t __sys_read(int fd, char* buffer, size_t size);
int64_t __sys_write(int fd, const char* buffer, size_t size);
int __sys_close(int fd);
size_t __sys_seek(int fd, size_t offset, int whence);
int __sys_ioctl(int fd, int command, void* arg);
int __sys_mkdir(const char* path, int mode);
int __sys_rmdir(const char* path);
int __sys_unlink(const char* path);
int __sys_stat(const char* path, void* stat);
int __sys_getdents(const char* path, void* entries, uint32_t count);

int __sys_exit(int status);
void* __sys_sbrk(intptr_t size);
int __sys_getpid();
int __sys_execve(const char *path, char *const argv[]);