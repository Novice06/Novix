#!/bin/bash

set -e

TARGET=$1
SIZE=$2
BUILD_DIR=$3

STAGE1_STAGE2_LOCATION_OFFSET=480

DISK_SECTOR_COUNT=$(( (${SIZE} + 511 ) / 512 ))

DISK_PART1_BEGIN=2048
DISK_PART1_END=$(( ${DISK_SECTOR_COUNT} - 1 ))

# generate image file
echo "Generating disk image ${TARGET} (${DISK_SECTOR_COUNT} sectors)..."
dd if=/dev/zero of=$TARGET bs=512 count=${DISK_SECTOR_COUNT} >/dev/null

# create partition table
echo "Creating partition..."
parted -s $TARGET mklabel msdos
parted -s $TARGET mkpart primary ${DISK_PART1_BEGIN}s ${DISK_PART1_END}s
parted -s $TARGET set 1 boot on

STAGE2_SIZE=$(stat -c%s ${BUILD_DIR}/boot0.bin)
echo ${STAGE2_SIZE}
STAGE2_SECTORS=$(( ( ${STAGE2_SIZE} + 511 ) / 512 ))
echo ${STAGE2_SECTORS}
echo $(( ${DISK_PART1_BEGIN} - 1 ))

if [ ${STAGE2_SECTORS} -gt $(( ${DISK_PART1_BEGIN} - 1 )) ]; then
    echo "Stage2 too big!!!"
    exit 2
fi

dd if=${BUILD_DIR}/boot0.bin of=$TARGET conv=notrunc bs=512 seek=1 #>/dev/null

# create loopback device
DEVICE=$(losetup -fP --show ${TARGET})
echo "Created loopback device ${DEVICE}"
TARGET_PARTITION="${DEVICE}p1"

# create file system
echo "Formatting ${TARGET_PARTITION}..."
mkfs.fat -F 32 -n "NOVIX" $TARGET_PARTITION >/dev/null

# install bootloader
echo "Installing bootloader on ${TARGET_PARTITION}..."
dd if=${BUILD_DIR}/bootloader.bin of=$TARGET_PARTITION conv=notrunc bs=1 count=3 2>&1 >/dev/null
dd if=${BUILD_DIR}/bootloader.bin of=$TARGET_PARTITION conv=notrunc bs=1 seek=90 skip=90 2>&1 >/dev/null

# write lba address of stage2 to bootloader
echo "01 00 00 00" | xxd -r -p | dd of=$TARGET_PARTITION conv=notrunc bs=1 seek=$STAGE1_STAGE2_LOCATION_OFFSET
printf "%x" ${STAGE2_SECTORS} | xxd -r -p | dd of=$TARGET_PARTITION conv=notrunc bs=1 seek=$(( $STAGE1_STAGE2_LOCATION_OFFSET + 4 ))

# copy files
echo "Copying files to ${TARGET_PARTITION} (mounted on ${BUILD_DIR}/tmp)..."
mkdir -p ${BUILD_DIR}/tmp
mount ${TARGET_PARTITION} ${BUILD_DIR}/tmp
cp ${BUILD_DIR}/kernel.bin ${BUILD_DIR}/tmp
mkdir ${BUILD_DIR}/tmp/test
umount ${BUILD_DIR}/tmp

# destroy loopback device
losetup -d ${DEVICE}