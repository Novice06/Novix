BUILD_DIR?=build
LIB_DIR?=lib

ASM?=nasm
ASMFLAGS= -f elf

LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I include -I $(abspath $(LIB_DIR))

LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)
LDFLAG-ELF=-T linker-elf.ld -nostdlib -L $(LIBGCC_PATH)


HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SOURCES_C =	$(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \
			hal/isrs_gen.c

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  hal/isrs_gen.inc

SOURCES_ASM =  $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) \

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

LIB_OBJECT =	$(wildcard $(BUILD_DIR)/lib/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*/*.obj) \

kernel: isr_generator $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf

isr_generator:
	@sed -i 's/\r//' hal/isr_generator.sh
	@./hal/isr_generator.sh hal/isrs_gen.c hal/isrs_gen.inc
	@echo "--> Generating: hal/isrs_gen.c hal/isrs_gen.inc"

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@ 
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(LD) $(LDFLAG) -o $@ $^ $(LIB_OBJECT) -lgcc -Map $(BUILD_DIR)/kernel.map
	@echo "--> Created:  kernel.bin"

$(BUILD_DIR)/kernel.elf: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(LD) $(LDFLAG-ELF) -o $@ $^ $(LIB_OBJECT) -lgcc
	@echo "--> Created:  kernel.elf (for debugging)"
