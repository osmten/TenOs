#include "ata.h"

// Small delay (required by ATA spec after some operations)
static inline void io_wait(void) {
    // Read status register 4 times (about 400ns delay)
    port_byte_in(ATA_PRIMARY_CONTROL);
    port_byte_in(ATA_PRIMARY_CONTROL);
    port_byte_in(ATA_PRIMARY_CONTROL);
    port_byte_in(ATA_PRIMARY_CONTROL);
}

// Wait until BSY (busy) bit is clear
static void ata_wait_bsy(void) {
    u8 status;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (status & ATA_SR_BSY);
}

// Wait until DRQ (data request) bit is set
static void ata_wait_drq(void) {
    u8 status;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRQ));
}

// Wait until drive is ready
static void ata_wait_ready(void) {
    u8 status;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRDY) || (status & ATA_SR_BSY));
}

// Check for errors
static bool ata_check_error(void) {
    u8 status = port_byte_in(ATA_PRIMARY_IO + ATA_REG_STATUS);
    
    if (status & ATA_SR_ERR) {
        u8 error = port_byte_in(ATA_PRIMARY_IO + ATA_REG_ERROR);
        pr_err("ATA", "ATA Error: %x", error);
        return true;
    }
    
    if (status & ATA_SR_DF) {
        pr_err("ATA", "Drive Fault!\n");
        return true;
    }
    
    return false;
}

// Initialize ATA controller
void ata_init(void) {
    printk("Initializing ATA controller...\n");

    port_byte_out(ATA_PRIMARY_CONTROL, 0x04);
    io_wait();
    port_byte_out(ATA_PRIMARY_CONTROL, 0x00);
    
    ata_wait_ready();
    
    printk("ATA controller initialized\n");
}

int ata_identify(void) {
    printk("Identifying ATA drive...\n");
    
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, ATA_DRIVE_MASTER);
    io_wait();
    
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, 0);
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_MID, 0);
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, 0);
    
    // Send IDENTIFY command
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();
    
    // Check if drive exists
    u8 status = port_byte_in(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0) {
        pr_err("ATA", "No drive detected\n");
        return -1;
    }
    
    ata_wait_bsy();
    
    if (ata_check_error()) {
        pr_err("ATA", "Drive detectction error\n");
        return -1;
    }
    
    ata_wait_drq();
    
    u16 identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = port_word_in(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    // Word 60-61: Total addressable sectors (28-bit LBA)
    u32 sectors = *(u32*)&identify_data[60];
    
    printk("Drive Selected: \n");
    printk("Total Sectors %d\n", sectors);
    
    return 0;
}

// Read a single sector using 28-bit LBA
void ata_read_sector(u32 lba, u8 *buffer) {
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    
    // Send READ SECTORS command
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    ata_wait_drq();
    
    // Check for errors
    if (ata_check_error()) {
        pr_err("ATA", "Error reading sector %d\n", lba);
        return;
    }
    
    u16 *buf16 = (u16*)buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = port_word_in(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    io_wait();
}

// Write a single sector using 28-bit LBA
void ata_write_sector(u32 lba, const u8 *buffer) {
    
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    // Send sector count (1 sector)
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    
    // Send WRITE SECTORS command
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    ata_wait_drq();
    
    const u16 *buf16 = (const u16*)buffer;
    for (int i = 0; i < 256; i++) {
        port_word_out(ATA_PRIMARY_IO + ATA_REG_DATA, buf16[i]);
    }
    
    io_wait();
    
    port_byte_out(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    ata_wait_bsy();
    
    // Check for errors
    if (ata_check_error()) {
        pr_err("ATA", "Error writing sector %d\n", lba);
    }
}

void ata_read_sectors(u32 lba, u8 sector_count, u8 *buffer) {
    for (u8 i = 0; i < sector_count; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

void ata_write_sectors(u32 lba, u8 sector_count, const u8 *buffer) {
    for (u8 i = 0; i < sector_count; i++) {
        ata_write_sector(lba + i, buffer + (i * 512));
    }
}

void read_sector(u32 lba, u8 *buffer) {
    ata_read_sector(lba, buffer);
}

void write_sector(u32 lba, const u8 *buffer) {
    ata_write_sector(lba, buffer);
}