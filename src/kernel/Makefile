BUILD_DIR?=build
LIB_DIR?=lib

ASM?=nasm
ASMFLAGS= -f elf

# LLVM toolchain
CC?=clang
LD?=ld.lld
TARGET=i686-elf

CFLAG=--target=$(TARGET) -ffreestanding -nostdlib -std=c99 -g -m32 -fno-stack-protector -I include -I $(abspath $(LIB_DIR))

LDFLAG=-T linker.ld -nostdlib -m elf_i386
LDFLAG-ELF=-T linker-elf.ld -nostdlib -m elf_i386


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
			  $(wildcard */*/*/*.asm)

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))
OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

LIB_OBJECT =	$(wildcard $(BUILD_DIR)/lib/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*/*.obj)

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


# Step 1: build ELF
$(BUILD_DIR)/kernel.elf: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(LD) $(LDFLAG-ELF) -o $@ $^ $(LIB_OBJECT) -Map $(BUILD_DIR)/kernel.map
	@echo "--> Created: kernel.elf"

# Step 2: generate flat binary from ELF
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	@llvm-objcopy -O binary $< $@
	@echo "--> Created: kernel.bin"

$(BUILD_DIR)/kernel.elf: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(LD) $(LDFLAG-ELF) -o $@ $^ $(LIB_OBJECT)
	@echo "--> Created:  kernel.elf (for debugging)"