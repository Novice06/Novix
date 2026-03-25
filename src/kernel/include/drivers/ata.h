/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_FEATURES     0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_SELECT 0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_PRIMARY_ALT_STATUS   0x3F6
#define ATA_PRIMARY_CONTROL      0x3F6
#define ATA_SECONDARY_DATA         0x170
#define ATA_SECONDARY_ERROR        0x171
#define ATA_SECONDARY_FEATURES     0x171
#define ATA_SECONDARY_SECTOR_COUNT 0x172
#define ATA_SECONDARY_LBA_LOW      0x173
#define ATA_SECONDARY_LBA_MID      0x174
#define ATA_SECONDARY_LBA_HIGH     0x175
#define ATA_SECONDARY_DRIVE_SELECT 0x176
#define ATA_SECONDARY_STATUS       0x177
#define ATA_SECONDARY_COMMAND      0x177
#define ATA_SECONDARY_ALT_STATUS   0x376
#define ATA_SECONDARY_CONTROL      0x376

// ata Commands
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_READ_SECTORS_EXT 0x24
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_WRITE_SECTORS_EXT 0x34
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_IDENTIFY          0xEC

// ata SRB
#define ATA_STATUS_ERR   (1 << 0)  // error
#define ATA_STATUS_IDX   (1 << 1)  // index
#define ATA_STATUS_CORR  (1 << 2)  // correct
#define ATA_STATUS_DRQ   (1 << 3)  // DRR
#define ATA_STATUS_DSC   (1 << 4)  // DSC
#define ATA_STATUS_DF    (1 << 5)  // D fault
#define ATA_STATUS_DRDY  (1 << 6)  // D ready
#define ATA_STATUS_BSY   (1 << 7)  // BSY

// ata CRB
#define ATA_CONTROL_NIEN (1 << 1)  // Disable interrupts
#define ATA_CONTROL_SRST (1 << 2)  // Software reset
#define ATA_CONTROL_HOB  (1 << 7)  // High Order Byte

// ata D select (a/b)
#define ATA_DRIVE_MASTER 0xA0
#define ATA_DRIVE_SLAVE  0xB0

// Sector size
#define ATA_SECTOR_SIZE 512 //(1 sek)

// timed out values(in loop iterations)
#define ATA_TIMEOUT_BSY 10000
#define ATA_TIMEOUT_DRQ 10000


typedef enum {
    ATA_DEVICE_NONE = 0,
    ATA_DEVICE_PATA,
    ATA_DEVICE_SATA,
    ATA_DEVICE_PATAPI,
    ATA_DEVICE_SATAPI
} ATAdevice_type_t;
typedef struct {
    uint16_t base_port;        // base io port (0x1F0 or 0x170)
    uint16_t control_port;     // control port (0x3F6 or 0x376)
    uint8_t is_slave;          // 0 = master, 1 = slave
    ATAdevice_type_t type;// type of devise
    uint32_t capabilities;     // Device capabilities
    char model[41];       // Model string
    char serial[21];      // Serial number
    uint8_t lba48_supported;   // LBA48 support flag
    uint64_t sectors;
} ATAdevice_t;
typedef struct {
    uint16_t config;              // W0
    uint16_t cylinders;           // W1
    uint16_t reserved1;           // W2
    uint16_t heads;               // W3
    uint16_t reserved2[2];        // W4-W5
    uint16_t sectors_per_track;   // W6
    uint16_t reserved3[3];        // W7-W9
    uint16_t serial[10];          // W10-W19 (20 ASCII characters)
    uint16_t reserved4[3];        // W20-W22
    uint16_t firmware_rev[4];     // W23-W26 (8 ASCII characters)
    uint16_t model[20];           // W27-W46 (40 ASCII characters)
    uint16_t max_multiple;        // W47
    uint16_t reserved5;           // W48
    uint16_t capabilities[2];     // W49-W50
    uint16_t reserved6[2];        // W51-W52
    uint16_t valid_fields;        // W53
    uint16_t reserved7[5];        // W54-W58
    uint16_t current_multiple;    // W59
    uint16_t lba28_sectors[2];    // W60-W61 (addressabel sectors(28))
    uint16_t reserved8[38];       // W62-W99
    uint16_t lba48_sectors[4];    // W100-W103 (addressabel sectors(48))
    uint16_t reserved9[152];      // W104-W255
} __attribute__((packed)) ATAidentify_t;

// pub
void ata_init(void);

int ATAdetect_devices(void);
ATAdevice_t* ATAget_device(int index);
int ATAget_device_count(void);

int ATAread_sectors(ATAdevice_t *dev, uint64_t lba, uint8_t sector_count, uint16_t *buffer);
int ATAwrite_sectors(ATAdevice_t *dev, uint64_t lba, uint8_t sector_count, uint16_t *buffer);

// timeout
int ATAwait_bsy(uint16_t base);
int ATAwait_drq(uint16_t base);
uint8_t ATAstatus_read(uint16_t base);
int ATAdevice_tidentify(ATAdevice_t *dev, ATAidentify_t *identify);
void ATAstring_swap(char *str, int len);
