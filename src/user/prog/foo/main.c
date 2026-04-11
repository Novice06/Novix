#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

/* Structures indispensables */
typedef struct {
    char     magic[4];      
    int32_t  numlumps;      
    int32_t  infotableofs;  
} t_wad_header;

typedef struct {
    int32_t  filepos;       
    int32_t  size;          
    char     name[8];       
} t_lump_info;

/* Framework de test ultra-basique */
void ok(const char* msg) { printf("[OK] %s\n", msg); }
void fail(const char* msg) { printf("[FAIL] %s\n", msg); }

int main(int argc, char **argv) {

    const char* path = "/doom1.wad";
    int fd;
    t_wad_header header;

    printf("--- TEST START: %s ---\n", path);

    /* 1. TEST OPEN */
    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        fail("open()");
        return 1;
    }
    ok("open()");

    /* 2. TEST LSEEK (Taille du fichier) */
    int32_t fsize = (int32_t)lseek(fd, 0, SEEK_END);
    if (fsize <= 0) {
        fail("lseek(SEEK_END)");
    } else {
        printf("[OK] lseek(SEEK_END) -> Size: %d bytes\n", fsize);
    }
    lseek(fd, 0, SEEK_SET);

    /* 3. TEST READ (Header) */
    int r = read(fd, &header, 12);
    if (r != 12) {
        fail("read(header)");
    } else {
        char magic[5] = {0};
        memcpy(magic, header.magic, 4);
        printf("[OK] read(header) -> Magic: %s, Lumps: %d, Offs: 0x%x\n", 
               magic, header.numlumps, header.infotableofs);
    }

    /* 4. TEST MALLOC */
    uint32_t dir_bytes = header.numlumps * sizeof(t_lump_info);
    t_lump_info* dir = (t_lump_info*)malloc(dir_bytes);
    if (!dir) {
        fail("malloc()");
        close(fd);
        return 1;
    }
    printf("[OK] malloc(%d bytes) at 0x%x\n", dir_bytes, (unsigned int)dir);

    /* 5. TEST LSEEK + READ (Directory) */
    lseek(fd, header.infotableofs, SEEK_SET);
    r = read(fd, dir, dir_bytes);
    if (r != (int)dir_bytes) {
        fail("read(directory)");
    } else {
        ok("read(directory)");
    }

    /* 6. TEST DATA INTEGRITY (Parcourir quelques lumps) */
    printf("Listing first 5 lumps:\n");
    for (int i = 0; i < (header.numlumps > 5 ? 5 : header.numlumps); i++) {
        char name[9] = {0};
        memcpy(name, dir[i].name, 8);
        printf("  Lump %d: %s (Offset: 0x%x, Size: %d)\n", 
               i, name, dir[i].filepos, dir[i].size);
        
        /* Verifier si le lump est dans le fichier */
        if (dir[i].filepos + dir[i].size > fsize) {
            printf("  [!] Error: Lump %d is out of bounds!\n", i);
        }
    }

    /* 7. TEST FREE & CLOSE */
    free(dir);
    ok("free()");
    
    close(fd);
    ok("close()");

    printf("--- ALL TESTS FINISHED ---\n");
    return 0;
}