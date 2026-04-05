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

#include <debug.h>
#include <hal/io.h>
#include <memory.h>
#include <string.h>
#include <drivers/ata.h>
#include <drivers/device.h>
#include <mem_manager/heap.h>

#define MAX_ATA_DEVICES 4

static ATAdevice_t ATAdevices[MAX_ATA_DEVICES];
static int ATAdevice_count = 0;

void io_wait()
{
    outb(0x80, 0);
}

int ATAwait_bsy(uint16_t base) {
    int timeout = ATA_TIMEOUT_BSY * 100; // increase timeout

    while (timeout > 0) {
        uint8_t status = inb(base + 7);
        if (!(status & ATA_STATUS_BSY)) {
            return 0; //s uccess
        }
        io_wait();
        timeout--;
    }

    return -1; // timed out
}

int ATAwait_drq(uint16_t base) {
    int timeout = ATA_TIMEOUT_DRQ * 100; // increase timeout

    while (timeout > 0) {
        uint8_t status = inb(base + 7);
        if (status & ATA_STATUS_DRQ) {
            return 0; // success
        }
        if (status & ATA_STATUS_ERR) {
            uint8_t err = inb(base + 1); // read error register
            log_err("[ATA]", "Error: ");
            return -2; // rrror
        }
        io_wait();
        timeout--;
    }

    return -1; // timed out
}


uint8_t ATAstatus_read(uint16_t base) {
    return inb(base + 7);
}

void ATAstring_swap(char *str, int len) {
    for (int i = 0; i < len; i += 2) {
        char tmp = str[i];
        str[i] = str[i + 1];
        str[i + 1] = tmp;
    }
}

// 400ns delay when reading alternate status
static void ATA400ns_delay(uint16_t control_port) {
    for (int i = 0; i < 14; i++) { // increase to 1000ns for safety
        inb(control_port);
    }
}

static void ATAsoft_reset(uint16_t control_port) { // (software resset)
    outb(control_port, ATA_CONTROL_SRST);
    ATA400ns_delay(control_port);
    outb(control_port, 0);
    ATA400ns_delay(control_port);
}

// ATA IDENTIFY command
int ATAidentify(ATAdevice_t *dev, ATAidentify_t *identify)
{
    uint16_t base = dev->base_port;
    uint8_t drive = dev->is_slave ? ATA_DRIVE_SLAVE : ATA_DRIVE_MASTER;

    // floating bus
    uint8_t status = inb(base + 7);
    if (status == 0xFF) {
        return -1; // floating bus and no device/S
    }

    // select drive
    outb(base + 6, drive);
    ATA400ns_delay(dev->control_port);

    // read status after selection
    status = inb(base + 7);
    if (status == 0 || status == 0xFF) {
        return -1; // no drive
    }

    // disable interrupts
    outb(dev->control_port, ATA_CONTROL_NIEN);

    // set sector count and LBA to 0
    outb(base + 2, 0);
    outb(base + 3, 0);
    outb(base + 4, 0);
    outb(base + 5, 0);

    // send ATA IDENTIFY command
    outb(base + 7, ATA_CMD_IDENTIFY);
    ATA400ns_delay(dev->control_port);

    // checks if drive exists
    status = inb(base + 7);
    if (status == 0) {
        return -1; // no drive
    }
    if (ATAwait_bsy(base) != 0) {
        printf("Timeout waiting for BSY\n");
        return -1;
    }

    // looks for ATAPI device
    uint8_t lba_mid = inb(base + 4);
    uint8_t lba_high = inb(base + 5);

    if (lba_mid != 0 || lba_high != 0) {
        // ATAPI device
        dev->type = (lba_mid == 0x14 && lba_high == 0xEB) ?
                    ATA_DEVICE_PATAPI : ATA_DEVICE_SATAPI;
        printf("ATAPI devices are not supported\n");
        return -2; // not supported yet
    }

    // waits until DRQ with timeout
    int drq_result = ATAwait_drq(base);
    if (drq_result != 0) {
        if (drq_result == -2) {
            printf("    Error flag set\n");
        } else {
            printf("    Timeout waiting for DRQ\n");
        }
        return -1;
    }

    // reads 256 words (512 bytes, 1sek)
    void *buffer = (void*)identify;
    for (int i = 0; i < 256; i++) {
        ((uint16_t*)buffer)[i] = inw(base);
    }

    return 0;
}

// detects ata device on specific bus
static int ATAdetect_device(uint16_t base_port, uint16_t control_port, uint8_t is_slave)
{
    if (ATAdevice_count >= MAX_ATA_DEVICES) {
        return -1;
    }

    // creates a temporary device structure
    ATAdevice_t temp_dev;
    memset(&temp_dev, 0, sizeof(ATAdevice_t));
    temp_dev.base_port = base_port;
    temp_dev.control_port = control_port;
    temp_dev.is_slave = is_slave;
    temp_dev.type = ATA_DEVICE_NONE;

    // this checks if the ports are responding
    uint8_t initial_status = inb(base_port + 7);
    if (initial_status == 0xFF) {
        return -1; // floating bus
    }

    ATAidentify_t identify;
    int result = ATAidentify(&temp_dev, &identify);
    if (result != 0) {
        return -1;
    }



    ATAdevice_t *dev = &ATAdevices[ATAdevice_count];
    memcpy(dev, &temp_dev, sizeof(ATAdevice_t));
    memcpy(dev->model, identify.model, 40);
    ATAstring_swap(dev->model, 40);
    dev->model[40] = '\0';

    // trimt railing spaces
    for (int i = 39; i >= 0; i--) {
        if (dev->model[i] == ' ') {
            dev->model[i] = '\0';
        } else {
            break;
        }
    }

    // identifis serial number
    memcpy(dev->serial, identify.serial, 20);
    ATAstring_swap(dev->serial, 20);
    dev->serial[20] = '\0';

    // trim trailing spaces from serial
    for (int i = 19; i >= 0; i--) {
        if (dev->serial[i] == ' ') {
            dev->serial[i] = '\0';
        } else {
            break;
        }
    }

    // LBA48 support
    dev->lba48_supported = (identify.capabilities[1] & (1 << 10)) ? 1 : 0;



    if (dev->lba48_supported)
    {
        dev->sectors = ((uint64_t)identify.lba48_sectors[3] << 48) |
                      ((uint64_t)identify.lba48_sectors[2] << 32) |
                      ((uint64_t)identify.lba48_sectors[1] << 16) |
                      ((uint64_t)identify.lba48_sectors[0]);
    } else {
        dev->sectors = ((uint32_t)identify.lba28_sectors[1] << 16) |
                      ((uint32_t)identify.lba28_sectors[0]);
    }


    dev->type = ATA_DEVICE_PATA;
    dev->capabilities = identify.capabilities[0];

    ATAdevice_count++;
    return 0;
}

int ATAdetect_devices(void)
{
    ATAdevice_count = 0;

    log_info("[ATA]", "Detecting devices...");

    // IDE ports (legacy!)
    uint16_t primary_base = 0x1F0;
    uint16_t primary_ctrl = 0x3F6;
    uint16_t secondary_base = 0x170;
    uint16_t secondary_ctrl = 0x376;

    // primary
    printf("      Checking Primary Master... ");
    if (ATAdetect_device(primary_base, primary_ctrl, 0) == 0) {
        printf(" -> Found: %s\n", ATAdevices[ATAdevice_count - 1].model);
    } else {
        printf(" -> None\n");
    }

    // primary slave
    printf("      Checking Primary Slave... ");
    if (ATAdetect_device(primary_base, primary_ctrl, 1) == 0) {
        printf(" -> Found: %s\n", ATAdevices[ATAdevice_count - 1].model);
    } else {
        printf(" -> None\n");
    }

    // secondary master
    printf("      Checking Secondary Master... ");
    if (ATAdetect_device(secondary_base, secondary_ctrl, 0) == 0) {
        printf(" -> Found: %s\n", ATAdevices[ATAdevice_count - 1].model);
    } else {
        printf("      -> None\n");
    }

    // secondary slave
    printf("      Checking Secondary Slave... ");
    if (ATAdetect_device(secondary_base, secondary_ctrl, 1) == 0) {
        printf(" -> Found: %s", ATAdevices[ATAdevice_count - 1].model);
    } else {
        printf(" -> None\n");
    }

    char buf[64];
    log_info("[ATA]", "Found %d device(s)", ATAdevice_count);

    return ATAdevice_count;
}

// LBA28 read sector
int ATAread_sectors(ATAdevice_t *dev, uint64_t lba, uint8_t sector_count, uint16_t *buffer)
{
    uint16_t base = dev->base_port;
    uint8_t drive = dev->is_slave ? ATA_DRIVE_SLAVE : ATA_DRIVE_MASTER;
    if (!dev || dev->type == ATA_DEVICE_NONE) {
        log_err("[ATA]", "Invalid device\n");
        return -1;
    }
    if (sector_count == 0) {
        log_err("[ATA]", "Zero sectors requested\n");
        return -1;
    }

    if (ATAwait_bsy(base) != 0) {
        log_err("[ATA]", "Initial BSY timeout\n");
        return -1;
    }

    // disables interups
    outb(dev->control_port, ATA_CONTROL_NIEN);
    ATA400ns_delay(dev->control_port);

    // select drive with right LBA mode
    outb(base + 6, drive | 0xE0 | ((lba >> 24) & 0x0F)); // 0xE0 is LBA
    ATA400ns_delay(dev->control_port);

    // waits abit
    if (ATAwait_bsy(base) != 0) {
        log_err("[ATA]", "BSY waiting\n");
        return -1;
    }

    outb(base + 2, sector_count);
    outb(base + 3, (uint8_t)(lba));
    outb(base + 4, (uint8_t)(lba >> 8));
    outb(base + 5, (uint8_t)(lba >> 16));
    outb(base + 7, ATA_CMD_READ_SECTORS);
    ATA400ns_delay(dev->control_port);

    for (int sector = 0; sector < sector_count; sector++)
    {
        // BSY clear
        if (ATAwait_bsy(base) != 0) {
            log_err("[ATA]", "BSY timeout in sector loop at sector %s\n", sector);
            return -1;
        }

        // DRQ
        int drq_result = ATAwait_drq(base);
        if (drq_result != 0) {
            uint8_t status = ATAstatus_read(base);
            log_err("[ATA]", "DRQ timeout at sector %d, status: %d", sector, status);
            return -1;
        }


        uint8_t status = ATAstatus_read(base);
        if (status & ATA_STATUS_ERR) {
            uint8_t err = inb(base + 1);
            log_err("[ATA]", "Error status at sector %d, error code: %d", sector, err);
            return -1;
        }

        // 256 words (512 bytes, 1 sek)
        for (int i = 0; i < 256; i++) {
            buffer[sector * 256 + i] = inw(base);
        }




        ATA400ns_delay(dev->control_port);
    }
return 0;
}
int ATAwrite_sectors(ATAdevice_t *dev, uint64_t lba, uint8_t sector_count, uint16_t *buffer)
{
    uint16_t base = dev->base_port;
    uint8_t drive = dev->is_slave ? ATA_DRIVE_SLAVE : ATA_DRIVE_MASTER;
    if (!dev || dev->type == ATA_DEVICE_NONE) {
        return -1;
    }
    if (sector_count == 0) {
        return -1;
    }
    if (ATAwait_bsy(base) != 0) {
        return -1;
    }

    // disables intersupts
    outb(dev->control_port, ATA_CONTROL_NIEN);
    ATA400ns_delay(dev->control_port);

    // selects lba mode with 0xE0
    outb(base + 6, drive | 0xE0 | ((lba >> 24) & 0x0F));
    ATA400ns_delay(dev->control_port);

    // waits after it
    if (ATAwait_bsy(base) != 0) {
        return -1;
    }


    outb(base + 2, sector_count);
    outb(base + 3, (uint8_t)(lba));
    outb(base + 4, (uint8_t)(lba >> 8));
    outb(base + 5, (uint8_t)(lba >> 16));
    outb(base + 7, ATA_CMD_WRITE_SECTORS);
    ATA400ns_delay(dev->control_port);

    for (int sector = 0; sector < sector_count; sector++)
    {
        if (ATAwait_bsy(base) != 0) {
            return -1;
        }
        if (ATAwait_drq(base) != 0) {
            return -1;
        }
        uint8_t status = ATAstatus_read(base);
        if (status & ATA_STATUS_ERR) {
            return -1;
        }

        // writes 256 words (512 bytes, 1 sek)
        for (int i = 0; i < 256; i++) {
            outw(base, buffer[sector * 256 + i]);
        }

        ATA400ns_delay(dev->control_port);
    }

    //flush cache
    if (ATAwait_bsy(base) == 0) {
        outb(base + 7, ATA_CMD_CACHE_FLUSH);
        ATAwait_bsy(base);
    }
    return 0;
}

// get device by index
ATAdevice_t* ATAget_device(int index) {
    if (index < 0 || index >= ATAdevice_count) {
        return NULL;
    }
    return &ATAdevices[index];
}
int ATAget_device_count(void) {
    return ATAdevice_count;
}

// -------------------------------------------------------------------

typedef struct
{
    uint8_t boot_flag;
    uint8_t start_head;
    uint16_t starting_sector_and_starting_cylinder;
    uint8_t system_id;
    uint8_t ending_head;
    uint16_t ending_sector_and_starting_cylinder;
    uint32_t relative_sector;
    uint32_t total_sectors;
} __attribute__((packed)) partition_entry_t;

typedef struct
{
    uint32_t relative_sector;
    ATAdevice_t *dev;
    // will be extended..., I guess
}device_info;

typedef struct
{
    uint8_t noise[512 - (sizeof(partition_entry_t) * 4) - 2];
    partition_entry_t entries[4];
    uint16_t boot_signature;
}__attribute__((packed)) mbr;

static int64_t read(uint8_t* buffer, int64_t offset , size_t len, void* dev, uint32_t flags);
static int64_t write(const uint8_t *buffer, int64_t offset, size_t len, void* dev, uint32_t flags);

void ata_init(void) {
    log_info("[ATA]", "Init ATA driver...\n");

    //ATAmodule_init();
    ATAdetect_devices();

    char* disks_names[] = {"sda", "sdb", "sdc", "sdd"};
    char* partitions_names[] = {"0", "1", "2", "3"};

    for ( int j = 0; j < ATAdevice_count; j++)     // just registering all disk and their partition as device
    {
        device_t* disk = kmalloc(sizeof(device_t));
        strcpy(disk->name, disks_names[j]);

        device_info* dev_info = kmalloc(sizeof(device_info));
        dev_info->relative_sector = 0;
        dev_info->dev = ATAget_device(0);

        disk->priv = dev_info;
        disk->read = read;
        disk->write = write;
        disk->ioctl = NULL;

        add_device(disk);

        mbr masterboot;
        void* buffer = &masterboot;
        ATAread_sectors(ATAget_device(0), 0, 1, (uint16_t*)buffer);

        uint8_t part_count = 0;
        for(int i = 0; i < 4; i++)      // ASSUME the disk is mbr partitioned !!
        {
            if(masterboot.entries[i].system_id == 0) continue; // empty entry

            device_t* disk = kmalloc(sizeof(device_t));
            char partname [16];
            strcpy(partname, disks_names[j]);
            strcat(partname, partitions_names[part_count++]);
            strcpy(disk->name, partname);

            device_info* dev_info = kmalloc(sizeof(device_info));
            dev_info->relative_sector = masterboot.entries[i].relative_sector;
            dev_info->dev = ATAget_device(0);

            disk->priv = dev_info;
            disk->read = read;
            disk->write = write;
            disk->ioctl = NULL;

            add_device(disk);
        }
    }
    
}

int64_t read(uint8_t* buffer, int64_t offset , size_t len, void* priv, uint32_t flags)
{
    device_info* dev_info = (void*)priv;
    return ATAread_sectors(dev_info->dev, offset + dev_info->relative_sector, len, (uint16_t*)buffer) == 0 ? (uint8_t)len : 0;
}

int64_t write(const uint8_t *buffer, int64_t offset, size_t len, void* priv, uint32_t flags)
{
    device_info* dev_info = (void*)priv;
    return ATAwrite_sectors(dev_info->dev, offset + dev_info->relative_sector, len, (uint16_t*)buffer) == 0 ? (uint8_t)len : 0;
}