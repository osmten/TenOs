#include "ata.h"
#include <stdint.h>
#include <stdbool.h>

// Port I/O helper functions
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value) {
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
    uint8_t status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (status & ATA_SR_BSY);
}

// Wait until DRQ (data request) bit is set
static void ata_wait_drq(void) {
    uint8_t status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRQ));
}

// Wait until drive is ready
static void ata_wait_ready(void) {
    uint8_t status;
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRDY) || (status & ATA_SR_BSY));
}

// Check for errors
static bool ata_check_error(void) {
    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    
    if (status & ATA_SR_ERR) {
        uint8_t error = inb(ATA_PRIMARY_IO + ATA_REG_ERROR);
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
    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
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
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    // Extract useful information
    // Word 60-61: Total addressable sectors (28-bit LBA)
    uint32_t sectors = *(uint32_t*)&identify_data[60];
    
    // print("Drive detected:\n");
    // print("  Total sectors: ");
    // print_hex(sectors);
    // print("\n  Capacity: ");
    // print_hex((sectors * 512) / (1024 * 1024));
    // print(" MB\n");
    
    return 0;
}

// Read a single sector using 28-bit LBA
void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    // Wait until drive is ready
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    // Send sector count (1 sector)
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    
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
    uint16_t *buf16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    // 400ns delay after reading
    io_wait();
}

// Write a single sector using 28-bit LBA
void ata_write_sector(uint32_t lba, const uint8_t *buffer) {
    // Wait until drive is ready
    ata_wait_bsy();
    
    // Select master drive + LBA mode + high 4 bits of LBA
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE_HEAD, 
         ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    
    // Send sector count (1 sector)
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    
    // Send LBA (bits 0-23)
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    
    // Send WRITE SECTORS command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    // Wait for drive to be ready for data
    ata_wait_drq();
    
    // Write 256 words (512 bytes)
    const uint16_t *buf16 = (const uint16_t*)buffer;
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
void ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    for (uint8_t i = 0; i < sector_count; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

// Write multiple sectors
void ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t *buffer) {
    for (uint8_t i = 0; i < sector_count; i++) {
        ata_write_sector(lba + i, buffer + (i * 512));
    }
}

// Wrapper functions for FAT12 driver compatibility
void read_sector(uint32_t lba, uint8_t *buffer) {
    ata_read_sector(lba, buffer);
}

void write_sector(uint32_t lba, const uint8_t *buffer) {
    ata_write_sector(lba, buffer);
}