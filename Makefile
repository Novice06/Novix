BUILD_DIR=build
SRC_DIR=src
FAT=mkfs.fat
ASM=nasm
CC=/home/novice/opt/cross/bin/i686-elf-gcc
LD=/home/novice/opt/cross/bin/i686-elf-ld
LIBGCC_PATH= /home/novice/opt/cross/lib/gcc/i686-elf/13.3.0
LIB_DIR=$(abspath $(SRC_DIR)/lib/)

export FAT
export ASM
export CC
export LD
export LIBGCC_PATH
export LIB_DIR

#
# Floppy image
#

floppy_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: lib bootloader kernel
	chmod +x make_disk.sh
	./make_disk.sh $@ 52428800 $(BUILD_DIR)

#
# lib
#

lib:
	$(MAKE) -C $(LIB_DIR) BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Bootloader
#

bootloader: stage1 stage2

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1/ BUILD_DIR=$(abspath $(BUILD_DIR))

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2/ BUILD_DIR=$(abspath $(BUILD_DIR))

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/ BUILD_DIR=$(abspath $(BUILD_DIR))

run: floppy_image
	chmod +x run.sh
	./run.sh

clean:
	rm -rf build/*