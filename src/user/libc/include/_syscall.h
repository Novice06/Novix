#pragma once

#include <stdint.h>
#include <stddef.h>

void __attribute__((cdecl)) __sys_sleep(uint32_t ms);
int __attribute__((cdecl))  __sys_open(const char* path, uint16_t mode);
int64_t __attribute__((cdecl)) __sys_read(int fd, char* buffer, uint64_t size);
int64_t __attribute__((cdecl)) __sys_write(int fd, const char* buffer, uint64_t size);
int __attribute__((cdecl)) __sys_close(int fd);
size_t __attribute__((cdecl)) __sys_seek(int fd, int64_t offset, int whence);
int __attribute__((cdecl)) __sys_ioctl(int fd, int command, void* arg);
int __attribute__((cdecl)) __sys_mkdir(const char* path, int mode);
int __attribute__((cdecl)) __sys_rmdir(const char* path);
int __attribute__((cdecl)) __sys_unlink(const char* path);
int __attribute__((cdecl)) __sys_stat(const char* path, void* stat);
int __attribute__((cdecl)) __sys_getdents(const char* path, void* entries, uint32_t count);

int __attribute__((cdecl)) __sys_exit(int status);
void* __attribute__((cdecl)) __sys_sbrk(intptr_t size);
int __attribute__((cdecl)) __sys_getpid();
int __attribute__((cdecl)) __sys_execve(const char *path, char *const argv[]);