BUILD_DIR?=build

ASM?=nasm
ASMFLAGS=-f elf32

# LLVM toolchain
CC?=clang
TARGET=i686-elf

CFLAG=--target=$(TARGET) -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -std=c99 -g

SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c,$(BUILD_DIR)/lib/c/%.obj,$(SOURCES_C))

lib: $(OBJECTS_C)

$(BUILD_DIR)/lib/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@
	@echo "--> Compiled: " $<