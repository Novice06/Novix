CC  ?= /home/novice/opt/cross/bin/i686-elf-gcc
AR  ?= /home/novice/opt/cross/bin/i686-elf-ar
BLD = build

CFLAGS := -ffreestanding -nostdlib -g -I include

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, $(BLD)/%.o, $(SRCS))

all: $(BLD)/libc.a $(BLD)/crt0.o

$(BLD)/libc.a: $(OBJS) $(BLD)/_syscall.o
	$(AR) rcs $@ $^

$(BLD)/%.o: %.c | $(BLD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/crt0.o: crt0.asm | $(BLD)
	nasm -f elf32 $< -o $@

$(BLD)/_syscall.o: _syscall.asm | $(BLD)
	nasm -f elf32 $< -o $@

$(BLD):
	mkdir -p $@

clean:
	rm -rf $(BLD)

.PHONY: all clean