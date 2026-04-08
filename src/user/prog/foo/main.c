#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

int main()
{
    printf("Hello, World!\n");

  
    DIR* dir = opendir("/");

    if(!dir)
    {
        printf("failed !!\n");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)))
    {
        printf("name: %s\n", entry->d_name);
    }
    
    void* mybuf = malloc(sizeof(char) * 65);

    FILE* file = fopen("/test/file.txt", "r+");
    fwrite("I feel good !!", sizeof(char), 13, file);
    fseek(file, 0, SEEK_SET);
    int read = fread(mybuf, sizeof(char), 64, file);
    ((char*)mybuf)[read] = '\0';
    printf("I read this: %s\n", mybuf);

    fseek(file, -15, SEEK_END);
    read = fread(mybuf, sizeof(char), 64, file);
    ((char*)mybuf)[read] = '\0';
    printf("I read this: %s\n", mybuf);

    mkdir("/my super cool directory");
    FILE* cool = fopen("/my super cool directory/cool.txt", "a+");
    fwrite("written something cool on this file !!", sizeof(char), 37, cool);

    return 0;
}