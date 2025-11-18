#include "ata.h"

// Port I/O helper functions
static inline u8 inb(u16 port) {
    u8 result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(u16 port, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(u16 port, u16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Small delay (required by ATA spec after some operations)
static inline void io_wait(void) {
    // Read status register 4 times (about 400ns delay)
    inb(ATA_PRIMARY_CONTROL);
    inb(ATA_PRIMARY_CONTROL);
    inb(ATA_PRIMARY_CONTROL);
    inb(ATA_PRIMARY_CONTROL);
}

// Wait until BSY (busy) bit is clear
static void ata_wait_bsy(void) {
    u8 status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (status & ATA_SR_BSY);
}

// Wait until DRQ (data request) bit is set
static void ata_wait_drq(void) {
    u8 status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRQ));
}

// Wait until drive is ready
static void ata_wait_ready(void) {
    u8 status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRDY) || (status & ATA_SR_BSY));
}

// Check for errors
static bool ata_check_error(void) {
    u8 status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    
    if (status & ATA_SR_ERR) {
        u8 error = inb(ATA_PRIMARY_IO + ATA_REG_ERROR);
        kprint("ATA Error: 0x");
        kprint(&error);
        kprint("\n");
        return true;
    }
    
    if (status & ATA_SR_DF) {
        kprint("ATA Drive Fault!\n");
        return true;
    }
    
    return false;
}

// Initialize ATA controller
void ata_init(void) {
    kprint("Initializing ATA controller...\n");
    
    // Software reset (optional but recommended)
    outb(ATA_PRIMARY_CONTROL, 0x04);  // Set SRST bit
    io_wait();
    outb(ATA_PRIMARY_CONTROL, 0x00);  // Clear SRST bit
    
    // Wait for drive to be ready
    ata_wait_ready();
    
    kprint("ATA controller initialized\n");
}

// Identify drive (optional - gets drive information)
int ata_identify(void) {
    // print("Identifying ATA drive...\n");
    
    // Select master drive
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, ATA_DRIVE_MASTER);
    io_wait();
    
    // Set sector count and LBA to 0
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, 0);
    
    // Send IDENTIFY command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();
    
    // Check if drive exists
    u8 status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0) {
        // print("No drive detected\n");
        return -1;
    }
    
    // Wait for BSY to clear
    ata_wait_bsy();
    
    // Check for errors
    if (ata_check_error()) {
        return -1;
    }
    
    // Wait for DRQ
    ata_wait_drq();
    
    // Read 256 words (512 bytes) of identification data
    u16 identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    // Extract useful information
    // Word 60-61: Total addressable sectors (28-bit LBA)
    u32 sectors = *(u32*)&identify_data[60];
    
    // print("Drive detected:\n");
    // print("  Total sectors: ");
    // print_hex(sectors);
    // print("\n  Capacity: ");
    // print_hex((sectors * 512) / (1024 * 1024));
    // print(" MB\n");
    
    return 0;
}

// Read a single sector using 28-bit LBA
void ata_read_sector(u32 lba, u8 *buffer) {
    // Wait until drive is ready
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    // Send sector count (1 sector)
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    
    // Send READ SECTORS command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    // Wait for drive to be ready with data
    ata_wait_drq();
    
    // Check for errors
    if (ata_check_error()) {
        kprint("Error reading sector ");
        // print_hex(lba);
        kprint("\n");
        return;
    }
    
    // Read 256 words (512 bytes)
    u16 *buf16 = (u16*)buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    // 400ns delay after reading
    io_wait();
}

// Write a single sector using 28-bit LBA
void ata_write_sector(u32 lba, const u8 *buffer) {
    // Wait until drive is ready
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    // Send sector count (1 sector)
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    
    // Send WRITE SECTORS command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    // Wait for drive to be ready for data
    ata_wait_drq();
    
    // Write 256 words (512 bytes)
    const u16 *buf16 = (const u16*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_IO + ATA_REG_DATA, buf16[i]);
    }
    
    // 400ns delay after writing
    io_wait();
    
    // Flush cache (ensure data is written to disk)
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    // Wait for flush to complete
    ata_wait_bsy();
    
    // Check for errors
    if (ata_check_error()) {
        kprint("Error writing sector ");
        kprint(&lba);
        kprint("\n");
    }
}

// Read multiple sectors (for efficiency)
void ata_read_sectors(u32 lba, u8 sector_count, u8 *buffer) {
    for (u8 i = 0; i < sector_count; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

// Write multiple sectors
void ata_write_sectors(u32 lba, u8 sector_count, const u8 *buffer) {
    for (u8 i = 0; i < sector_count; i++) {
        ata_write_sector(lba + i, buffer + (i * 512));
    }
}

// Wrapper functions for FAT12 driver compatibility
void read_sector(u32 lba, u8 *buffer) {
    ata_read_sector(lba, buffer);
}

void write_sector(u32 lba, const u8 *buffer) {
    ata_write_sector(lba, buffer);
}