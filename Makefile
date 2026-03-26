BUILD_DIR=build
SRC_DIR=src

FAT=mkfs.fat
ASM=nasm

# LLVM toolchain
CC=clang
LD=ld.lld

# Target architecture
TARGET=i686-elf

# Compiler flags
CFLAGS=--target=$(TARGET) -ffreestanding -nostdlib -fno-stack-protector -m32
LDFLAGS=-m elf_i386

LIB_DIR=$(abspath $(SRC_DIR)/lib/)

export FAT
export ASM
export CC
export LD
export TARGET
export CFLAGS
export LDFLAGS
export LIB_DIR


# ----------------------
# disk image
# ----------------------

disk_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: lib bootloader kernel
	chmod +x make_disk.sh
	./make_disk.sh $@ 52428800 $(BUILD_DIR)


# ----------------------
# bootable usb
# ----------------------

bootable_usb: lib bootloader kernel
	chmod +x make_bootable_usb.sh
	./make_bootable_usb.sh $(BUILD_DIR)


# ----------------------
# lib
# ----------------------

lib:
	$(MAKE) -C $(LIB_DIR) BUILD_DIR=$(abspath $(BUILD_DIR))


# ----------------------
# Bootloader
# ----------------------

bootloader: stage1 stage2

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1/ BUILD_DIR=$(abspath $(BUILD_DIR))

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2/ BUILD_DIR=$(abspath $(BUILD_DIR))


# ----------------------
# Kernel
# ----------------------

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/ BUILD_DIR=$(abspath $(BUILD_DIR))


# ----------------------
# Run in emulator
# ----------------------

run: disk_image
	chmod +x run.sh
	./run.sh


# ----------------------
# Clean
# ----------------------

clean:
	rm -rf build/*