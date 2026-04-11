#pragma once

typedef enum {
    O_RDONLY    = 1 << 0,   // read only
    O_WRONLY    = 1 << 1,   // write only
    O_RDWR      = 1 << 2,   // read / write
    O_APPEND    = 1 << 3,   // add at the end of file
    O_CREAT     = 1 << 4,   // create if doesnt exist
    O_TRUNC     = 1 << 5,   // trunc the file to 0
    O_NONBLOCK  = 1 << 15,  // non block operation 
} VfsFlag;

