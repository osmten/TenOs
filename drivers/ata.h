#ifndef ATA_H
#define ATA_H

#include <lib/lib.h>
#include <stdbool.h>

// ATA Status Register bits
#define ATA_SR_BSY      0x80    // Busy
#define ATA_SR_DRDY     0x40    // Drive ready
#define ATA_SR_DF       0x20    // Drive write fault
#define ATA_SR_DSC      0x10    // Drive seek complete
#define ATA_SR_DRQ      0x08    // Data request ready
#define ATA_SR_CORR     0x04    // Corrected data
#define ATA_SR_IDX      0x02    // Index
#define ATA_SR_ERR      0x01    // Error

// ATA Error Register bits
#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// ATA Commands
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC

// ATA Ports (Primary channel)
#define ATA_PRIMARY_IO          0x1F0
#define ATA_PRIMARY_CONTROL     0x3F6
#define ATA_PRIMARY_IRQ         14

// ATA Ports (Secondary channel)
#define ATA_SECONDARY_IO        0x170
#define ATA_SECONDARY_CONTROL   0x376
#define ATA_SECONDARY_IRQ       15

// Register offsets from base I/O port
#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECCOUNT        0x02
#define ATA_REG_LBA_LOW         0x03
#define ATA_REG_LBA_MID         0x04
#define ATA_REG_LBA_HIGH        0x05
#define ATA_REG_DRIVE_HEAD      0x06
#define ATA_REG_STATUS          0x07
#define ATA_REG_COMMAND         0x07

// Drive selection
#define ATA_DRIVE_MASTER        0xE0
#define ATA_DRIVE_SLAVE         0xF0

// Function prototypes
void ata_init(void);
void ata_read_sector(u32 lba, u8 *buffer);
void ata_write_sector(u32 lba, const u8 *buffer);
void ata_read_sectors(u32 lba, u8 sector_count, u8 *buffer);
void ata_write_sectors(u32 lba, u8 sector_count, const u8 *buffer);
int ata_identify(void);
void read_sector(u32 lba, u8 *buffer);
void write_sector(u32 lba, const u8 *buffer);

#endif