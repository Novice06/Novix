#pragma once
#include <sys/types.h>

// d_type values (POSIX / same as Linux)
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2 // character device
#define DT_DIR      4 // directory
#define DT_BLK      6 // block device
#define DT_REG      8 // regular file
#define DT_LNK      10 // symbolic link
#define DT_SOCK     12 // socket
#define DT_WHT      14 // whiteout

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];// null-terminated filename
};
typedef struct __dir DIR;

DIR *opendir (const char *path);
struct dirent *readdir (DIR *dirp);
int closedir(DIR *dirp);

// rewind to beginning
void rewinddir(DIR *dirp);