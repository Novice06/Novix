BUILD_DIR ?= build

ASM ?= nasm
ASMFLAGS ?= -f bin      # flat binary for stage1

# LLVM toolchain (for later C stages)
CC ?= clang
LD ?= ld.lld
TARGET ?= i686-elf

# Stage1 target
stage1: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: boot.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) boot.asm -o $(BUILD_DIR)/bootloader.bin
	@echo "--> Created: bootloader.bin"