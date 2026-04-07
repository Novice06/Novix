#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "_syscall.h"

// entry written by __sys_getdents
typedef struct {
    uint8_t _type; // file / dir
    char _name[256];
} _kdirent_t;

// max entries per directory read
// kept small so the stack buffer stays 4KB
#define _KDIRENT_MAX 64

static unsigned char _ktype_to_dt(unsigned char ktype) {
    switch (ktype) {
        case 0x01: return DT_REG;
        case 0x02: return DT_DIR;
        case 0x04: return DT_CHR;
        default:   return DT_UNKNOWN;
    }
}

struct __dir {
    struct dirent *_entries;
    int            _count;
    int            _pos;
};

DIR *opendir(const char *path)
{
    if (!path) { errno = EINVAL; return NULL; }

    // use a STACK buffer for the raw kernel data
    // free automatically when opendir returns
    _kdirent_t raw[_KDIRENT_MAX];

    long n = __sys_getdents(path, raw, _KDIRENT_MAX);
    if (n < 0) { errno = ENOENT; return NULL; }

    // only allocate heap for the actual entries we got back
    struct dirent *entries = NULL;
    if (n > 0) {
        entries = (struct dirent *)malloc(sizeof(struct dirent) * (int)n);
        if (!entries) { errno = ENOMEM; return NULL; }

        for (int i = 0; i < (int)n; i++) {
            entries[i].d_ino = 0;
            entries[i].d_off = 0;
            entries[i].d_reclen = (unsigned short)sizeof(struct dirent);
            entries[i].d_type = _ktype_to_dt(raw[i]._type);
            strncpy(entries[i].d_name, raw[i]._name, 255);
            entries[i].d_name[255] = '\0';
        }
    }

    DIR *dir = (DIR *)malloc(sizeof(DIR));
    if (!dir) {
        free(entries);
        errno = ENOMEM;
        return NULL;
    }

    dir->_entries = entries;
    dir->_count = (int)n;
    dir->_pos = 0;

    return dir;
}

struct dirent *readdir(DIR *dirp){
    if (!dirp) { errno = EBADF; return NULL; }
    if (dirp->_pos >= dirp->_count) return NULL;
    return &dirp->_entries[dirp->_pos++];
}

int closedir(DIR *dirp) {
    if (!dirp) { errno = EBADF; return -1; }
    free(dirp->_entries);
    free(dirp);
    return 0;
}

void rewinddir(DIR *dirp){ if (dirp) dirp->_pos = 0;}