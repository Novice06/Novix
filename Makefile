BUILD_DIR=$(abspath build)
SRC_DIR=src
FAT=mkfs.fat
ASM=nasm
CC=/home/novice/opt/cross/bin/i686-elf-gcc
LD=/home/novice/opt/cross/bin/i686-elf-ld
AR= /home/novice/opt/cross/bin/i686-elf-ar
LIBGCC_PATH= /home/novice/opt/cross/lib/gcc/i686-elf/13.3.0
LIB_DIR=$(abspath $(SRC_DIR)/lib/)

export FAT
export ASM
export CC
export LD
export AR
export LIBGCC_PATH
export LIB_DIR
export BUILD_DIR

#
# disk image
#

disk_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: lib bootloader kernel user
	chmod +x make_disk.sh
	./make_disk.sh $@ 52428800 $(BUILD_DIR)

#
# bootable usb
#

bootable_usb: lib bootloader kernel user
	chmod +x make_bootable_usb.sh
	./make_bootable_usb.sh $(BUILD_DIR)

#
# lib
#

lib:
	$(MAKE) -C $(LIB_DIR)

#
# Bootloader
#

bootloader: stage1 stage2

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1/

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2/

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/

user:
	$(MAKE) -C $(SRC_DIR)/user/

run: disk_image
	chmod +x run.sh
	./run.sh

clean:
	$(MAKE) -C $(SRC_DIR)/user/ clean
	rm -rf build/*